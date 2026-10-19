#include "wifi_setup.h"

#include <Preferences.h>
#include <WiFi.h>

static const char* PREFS_NAMESPACE = "wifi";

static String htmlEscape(const String& s) {
  String out;
  out.reserve(s.length());
  for (char c : s) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c;
    }
  }
  return out;
}

static const char PAGE_HEAD[] PROGMEM = R"(<!DOCTYPE html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>BeamNG Dash</title>
<style>body{font-family:sans-serif;background:#111;color:#eee;max-width:420px;margin:auto;padding:16px}
input,button{width:100%;box-sizing:border-box;padding:10px;margin:6px 0 14px;font-size:16px;border-radius:6px;border:1px solid #555}
input{background:#222;color:#eee}button{background:#e85d04;color:#fff;border:0}</style></head><body>)";

// ---------------------------------------------------------------------

WifiSetup::Mode WifiSetup::begin() {
  WiFi.persistent(false);              // dane sieci trzymamy sami w Preferences
  WiFi.setHostname(WIFI_HOSTNAME);

  String ssid, pass;
  if (loadCredentials(ssid, pass) && connect(ssid, pass)) {
    _mode = Mode::Station;
    WiFi.setAutoReconnect(true);
    WiFi.setSleep(false);              // bez oszczędzania energii = mniejsze opóźnienia UDP
  } else {
    startPortal();
    _mode = Mode::AccessPoint;
  }
  return _mode;
}

void WifiSetup::loop() {
  if (_mode != Mode::AccessPoint) return;

  _dns.processNextRequest();
  _server.handleClient();

  if (_restartAt && (int32_t)(millis() - _restartAt) >= 0) ESP.restart();
}

IPAddress WifiSetup::ip() const {
  return _mode == Mode::Station ? WiFi.localIP() : WiFi.softAPIP();
}

String WifiSetup::ssid() const {
  return _mode == Mode::Station ? WiFi.SSID() : String(WIFI_AP_SSID);
}

String WifiSetup::savedSsid() {
  String ssid, pass;
  return loadCredentials(ssid, pass) ? ssid : String();
}

void WifiSetup::clearCredentials() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}

// ---------------------------------------------------------------------

bool WifiSetup::loadCredentials(String& ssid, String& pass) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, true)) return false;   // namespace jeszcze nie istnieje
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  return ssid.length() > 0;
}

void WifiSetup::saveCredentials(const String& ssid, const String& pass) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
}

bool WifiSetup::connect(const String& ssid, const String& pass) {
  Serial.printf("[WiFi] Laczenie z \"%s\"...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), pass.c_str());

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
      Serial.println("[WiFi] Nie udalo sie polaczyc");
      WiFi.disconnect(true);
      return false;
    }
    delay(100);
  }
  Serial.printf("[WiFi] Polaczono, IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void WifiSetup::startPortal() {
  // STA potrzebne do skanowania sieci – lista trafia do podpowiedzi w formularzu
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  int found = WiFi.scanNetworks();
  _networkOptions = "";
  for (int i = 0; i < found; i++) {
    String name = WiFi.SSID(i);
    if (name.isEmpty()) continue;
    String option = "<option value=\"" + htmlEscape(name) + "\">";
    if (_networkOptions.indexOf(option) < 0) _networkOptions += option;   // bez duplikatów
  }
  WiFi.scanDelete();

  WiFi.softAP(WIFI_AP_SSID, strlen(WIFI_AP_PASSWORD) ? WIFI_AP_PASSWORD : nullptr);
  IPAddress apIp = WiFi.softAPIP();
  Serial.printf("[WiFi] Hotspot \"%s\", strona: http://%s/\n", WIFI_AP_SSID, apIp.toString().c_str());

  // DNS odpowiada na każde zapytanie adresem ESP -> telefon sam otworzy stronę (captive portal)
  _dns.start(53, "*", apIp);

  _server.on("/", HTTP_GET, [this] { handleRoot(); });
  _server.on("/save", HTTP_POST, [this] { handleSave(); });
  _server.onNotFound([this] { handleRedirect(); });
  _server.begin();
}

// ---------------------------------------------------------------------

void WifiSetup::handleRoot() {
  String page = FPSTR(PAGE_HEAD);
  page += F("<h2>BeamNG Dash &ndash; WiFi</h2><form method=\"post\" action=\"/save\">"
            "<label>Sieć (SSID)</label><input name=\"ssid\" list=\"nets\" required autocomplete=\"off\">"
            "<datalist id=\"nets\">");
  page += _networkOptions;
  page += F("</datalist><label>Hasło</label><input name=\"pass\" type=\"password\">"
            "<button type=\"submit\">Zapisz i uruchom ponownie</button></form></body></html>");
  _server.send(200, "text/html; charset=utf-8", page);
}

void WifiSetup::handleSave() {
  String ssid = _server.arg("ssid");
  String pass = _server.arg("pass");
  ssid.trim();
  if (ssid.isEmpty()) {
    _server.send(400, "text/plain; charset=utf-8", "Brak nazwy sieci");
    return;
  }

  saveCredentials(ssid, pass);
  String page = FPSTR(PAGE_HEAD);
  page += "<h2>Zapisano</h2><p>Urządzenie łączy się z siecią <b>" + htmlEscape(ssid) +
          "</b>. Adres IP pokaże się na wyświetlaczu.</p></body></html>";
  _server.send(200, "text/html; charset=utf-8", page);

  _restartAt = millis() + 2000;   // chwila na wysłanie odpowiedzi do przeglądarki
  if (_restartAt == 0) _restartAt = 1;
}

void WifiSetup::handleRedirect() {
  _server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  _server.send(302, "text/plain", "");
}
