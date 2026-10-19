#include "dashboard.h"

#include "lang.h"
#include "screens/widgets.h"
#include "wifi_setup.h"

static const int FRAME_MS = 33;              // ~30 klatek/s
static const float MIN_MAX_RPM = 6000.0f;    // startowy zakres obrotów
static const int TOUCH_POLL_MS = 25;         // co ile sprawdzamy dotyk

Dashboard::Dashboard(TFT_eSPI& tft, RgbLed& led, Touch& touch, Backlight& backlight)
    : _tft(tft),
      _led(led),
      _touch(touch),
      _race(tft),
      _gauges(tft),
      _engine(tft),
      _pedals(tft),
      _truck(tft),
      _config(tft, touch, backlight),
      _maxRpm(MIN_MAX_RPM) {}

void Dashboard::begin() {
  _tft.init();
  _tft.setRotation(1);
  _tft.fillScreen(TFT_BLACK);
}

void Dashboard::loop() {
  uint32_t now = millis();
  pollTouch(now);

  if (!_hasData) {
    _led.set(0, 0, 30);   // czekamy na dane – przygaszony niebieski
    return;
  }
  if ((!_screenActive && !_configOpen) || now - _lastFrameMs < FRAME_MS) return;
  _lastFrameMs = now;

  float frac = _data.rpm / _maxRpm;
  Telemetry t{_data, *_game, _maxRpm, frac, _topSpeed, (_data.lights & L_SHIFT) || frac >= 0.95f, (now / 100) % 2 == 1};
  if (_screenActive) currentScreen()->update(t);
  updateLed(t);   // shift light działa także w konfiguracji
}

Screen* Dashboard::screen(ScreenId id) {
  switch (id) {
    case ScreenId::Gauges: return &_gauges;
    case ScreenId::Engine: return &_engine;
    case ScreenId::Pedals: return &_pedals;
    case ScreenId::Truck: return &_truck;
    default: return &_race;
  }
}

Screen* Dashboard::currentScreen() {
  return screen(_game->screens[_current]);
}

void Dashboard::enterScreen() {
  currentScreen()->enter();
  _screenActive = true;
  _lastFrameMs = 0;   // pierwsza klatka od razu
}

// ---------------------------------------------------------------------
//  Zdarzenia z GameReceiver

void Dashboard::onTelemetry(const GameDef& game, const CarData& data) {
  if (&game != _game) {
    // inna gra -> jej lista ekranów, od pierwszego
    if (_screenActive) {
      currentScreen()->leave();
      _screenActive = false;
    }
    _game = &game;
    _current = 0;
    _carKey = ~data.carKey;   // wymusza reset zakresów poniżej
  }

  // Inne auto -> zakresy od nowa. Gry bez max obrotów (OPT_AUTO_RPM, np. BeamNG):
  // zakres = największe obroty widziane w tym aucie, min. 6000.
  if (data.carKey != _carKey) {
    _carKey = data.carKey;
    _maxRpm = MIN_MAX_RPM;
    _topSpeed = 0;
    Serial.printf("[Dashboard] Inne auto (0x%08x) - zakres obrotow od nowa\n", data.carKey);
  }
  // isfinite: zła wartość nie może utknąć w zakresie (CarData i tak przechodzi przez sanitize)
  if ((_game->options & OPT_AUTO_RPM) || !(data.rpmMax > 0) || !isfinite(data.rpmMax)) {
    if (isfinite(data.rpm) && data.rpm > _maxRpm) _maxRpm = data.rpm;
  } else {
    _maxRpm = data.rpmMax;
  }
  if (data.speed > _topSpeed) _topSpeed = data.speed;

  _data = data;
  _hasData = true;
  _hasGame = true;

  if (!_screenActive && !_configOpen) enterScreen();
}

void Dashboard::onTelemetryLost() {
  // przerwa w danych (menu, wybór auta, pauza) -> po powrocie zakres od nowa
  _maxRpm = MIN_MAX_RPM;
  _topSpeed = 0;
  _hasData = false;
  if (_configOpen) _info = Info::Waiting;   // pokaże się po wyjściu z konfiguracji
  else showWaiting(_waitIp, _waitPort);
}

// ---------------------------------------------------------------------
//  Zdarzenia z InReceiver (przyciski)

void Dashboard::PutButton1() {
  if (_configOpen) {
    handleConfig(_config.button(1));
    return;
  }
  switchScreen(1);
}

// step = 1 następny, -1 poprzedni; po ostatnim wraca do pierwszego i odwrotnie
void Dashboard::switchScreen(int step) {
  int count = _game->screenCount();
  uint8_t next = (_current + count + step) % count;
  if (_screenActive) {
    currentScreen()->leave();
    _current = next;
    enterScreen();
  } else {
    _current = next;   // pokaże się, gdy przyjdą dane z gry
  }
}

void Dashboard::PutButton2() {
  if (_configOpen) handleConfig(_config.button(2));
  else openConfig();
}

void Dashboard::PutButton3() {
  if (_configOpen) handleConfig(_config.button(3));
}

void Dashboard::PutButton4() {
  if (_configOpen) handleConfig(_config.button(4));
}

void Dashboard::PutButton4Long() {
  if (_configOpen) return;   // w konfiguracji WiFi kasuje się opcją z listy
  resetWifi();
}

// ---------------------------------------------------------------------
//  Ekran konfiguracji

void Dashboard::pollTouch(uint32_t now) {
  if (now - _lastTouchMs < TOUCH_POLL_MS) return;
  _lastTouchMs = now;

  uint16_t x, y;
  bool down = _touch.read(x, y);
  bool tap = down && !_touchDown;   // reagujemy tylko na początek dotyku
  _touchDown = down;
  if (!tap) return;

  if (_configOpen) {
    handleConfig(_config.tap(x, y));
    _touchDown = true;   // po akcji (np. kalibracji) czekamy na puszczenie palca
  } else if (_screenActive && ui::configIconHit(x, y)) {
    openConfig();
  } else if (_screenActive && y >= TOUCH_NAV_TOP && y < TOUCH_NAV_BOTTOM) {
    // środek lewej/prawej krawędzi ekranu auta = poprzedni/następny ekran
    if (x < TOUCH_NAV_WIDTH) switchScreen(-1);
    else if (x >= 320 - TOUCH_NAV_WIDTH) switchScreen(1);
  }
}

void Dashboard::openConfig() {
  if (_screenActive) {
    currentScreen()->leave();
    _screenActive = false;
  }
  _configOpen = true;
  _config.enter();
}

void Dashboard::closeConfig() {
  _configOpen = false;
  if (_hasData) enterScreen();
  else if (_info == Info::Portal) showPortal(_apSsid, _apPassword, _apIp);
  else showWaiting(_waitIp, _waitPort);
}

void Dashboard::handleConfig(ConfigScreen::Result r) {
  if (r == ConfigScreen::Result::Close) closeConfig();
  else if (r == ConfigScreen::Result::ResetWifi) resetWifi();
  else if (r == ConfigScreen::Result::FactoryReset) factoryReset();
}

void Dashboard::resetWifi() {
  // kasuje zapisaną sieć i restartuje – bez danych WiFi urządzenie startuje jako hotspot
  Serial.println("[WiFi] Kasowanie zapisanej sieci, restart");
  _hasData = false;
  _led.set(30, 0, 30);
  messageScreen(tr(S_WIFI_ERASING));
  _tft.drawString(tr(S_RESTART_HOTSPOT), 160, 110, 2);
  WifiSetup::clearCredentials();
  delay(1500);   // chwila, żeby przeczytać komunikat
  ESP.restart();
}

void Dashboard::factoryReset() {
  // kasuje wszystkie ustawienia z NVS: WiFi, kalibrację dotyku, jasność, język
  Serial.println("[Reset] Kasowanie ustawien, restart");
  _hasData = false;
  _led.set(30, 0, 30);
  messageScreen(tr(S_FACTORY_ERASING));
  _tft.drawString(tr(S_RESTARTING), 160, 110, 2);
  WifiSetup::clearCredentials();
  Touch::clearCalibration();
  Backlight::clear();
  lang::clear();
  delay(1500);   // chwila, żeby przeczytać komunikat
  ESP.restart();
}

// ---------------------------------------------------------------------
//  Ekrany informacyjne

void Dashboard::messageScreen(const char* title) {
  _configOpen = false;
  if (_screenActive) {
    currentScreen()->leave();
    _screenActive = false;
  }
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TC_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(title, 160, 20, 4);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
}

void Dashboard::showConnecting(const String& ssid) {
  messageScreen(tr(S_CONNECTING));
  _tft.drawString(ssid, 160, 100, 4);
}

void Dashboard::showPortal(const String& apSsid, const String& apPassword, const String& ip) {
  _info = Info::Portal;
  _apSsid = apSsid;
  _apPassword = apPassword;
  _apIp = ip;
  messageScreen(tr(S_WIFI_SETUP));
  _tft.drawString(tr(S_PORTAL_JOIN), 160, 64, 2);
  _tft.setTextColor(TFT_CYAN, TFT_BLACK);
  _tft.drawString(apSsid, 160, 84, 4);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (apPassword.length()) _tft.drawString(tr(S_PORTAL_PASSWORD) + apPassword, 160, 114, 2);
  _tft.drawString(tr(S_PORTAL_OPEN), 160, 150, 2);
  _tft.setTextColor(TFT_CYAN, TFT_BLACK);
  _tft.drawString("http://" + ip, 160, 170, 4);
}

void Dashboard::showWaiting(const String& ip, uint16_t port) {
  _info = Info::Waiting;
  _waitIp = ip;
  _waitPort = port;
  messageScreen(tr(S_WAITING));
  // podpowiedzi z listy gier, bez powtórzeń (ETS2 i ATS mają wspólną)
  int y = 54;
  for (int i = 0; i < GAME_COUNT; i++) {
    bool repeated = false;
    for (int j = 0; j < i; j++) repeated |= GAMES[i].setupHint == GAMES[j].setupHint;
    if (repeated) continue;
    _tft.drawString(tr(GAMES[i].setupHint), 160, y, 2);
    y += 18;
  }
  if (_hasGame) {
    _tft.setTextColor(ui::LABEL, TFT_BLACK);
    _tft.drawString(String(tr(S_LAST_GAME)) + _game->name, 160, 92, 2);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  }
  _tft.drawString(tr(S_IP), 160, 110, 2);
  _tft.setTextColor(TFT_CYAN, TFT_BLACK);
  _tft.drawString(ip, 160, 128, 4);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(S_PORT), 160, 166, 2);
  _tft.setTextColor(TFT_CYAN, TFT_BLACK);
  _tft.drawString(String(port), 160, 184, 4);
}

void Dashboard::showOtaStart() {
  _hasData = false;
  _led.set(30, 0, 30);
  messageScreen(tr(S_OTA_TITLE));
  _tft.drawString(tr(S_OTA_KEEP_POWER), 160, 70, 2);
  _tft.drawRect(30, 110, 260, 24, TFT_WHITE);
  showOtaProgress(0);
}

void Dashboard::showOtaProgress(uint8_t percent) {
  // onProgress przychodzi co kilka kB – rysujemy tylko przy zmianie procentu
  static uint8_t last = 255;
  if (percent == last) return;
  last = percent;
  _tft.fillRect(32, 112, 256 * percent / 100, 20, TFT_ORANGE);
  _tft.setTextDatum(TC_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.setTextPadding(80);
  _tft.drawString(String(percent) + "%", 160, 150, 4);
}

void Dashboard::showOtaError() {
  _led.set(255, 0, 0);
  messageScreen(tr(S_OTA_ERROR));
  _tft.drawString(tr(S_RESTARTING), 160, 110, 2);
}

// ---------------------------------------------------------------------

void Dashboard::updateLed(const Telemetry& t) {
  if (t.shift) _led.set(t.blinkOn ? 255 : 0, 0, 0);
  else if (t.rpmFrac >= 0.85f) _led.set(255, 0, 0);
  else if (t.rpmFrac >= 0.70f) _led.set(255, 120, 0);
  else if (t.rpmFrac >= 0.50f) _led.set(0, 255, 0);
  else _led.off();
}
