// Połączenie z siecią WiFi.
//  - są zapisane dane sieci -> łączy się jako klient (STA)
//  - brak danych albo połączenie się nie udało -> hotspot (AP) ze stroną www
//    do wpisania SSID i hasła; po zapisaniu urządzenie się restartuje
#pragma once

#include <Arduino.h>
#include <FS.h>
// TFT_eSPI dołącza FS.h z FS_NO_GLOBALS, a WebServer.h używa globalnego FS
using fs::FS;
#include <DNSServer.h>
#include <WebServer.h>

class WifiSetup {
public:
  enum class Mode { Station, AccessPoint };

  // Blokuje do czasu połączenia lub uruchomienia hotspotu.
  Mode begin();
  // Wywoływać w loop() – w trybie hotspot obsługuje stronę konfiguracji.
  void loop();

  Mode mode() const { return _mode; }
  IPAddress ip() const;
  String ssid() const;   // sieć, do której jesteśmy podłączeni, albo nazwa hotspotu

  // Nazwa zapisanej sieci ("" jeśli brak).
  static String savedSsid();
  // Kasuje zapisane dane sieci (np. pod przycisk) – po restarcie ruszy hotspot.
  static void clearCredentials();

private:
  static bool loadCredentials(String& ssid, String& pass);
  static void saveCredentials(const String& ssid, const String& pass);
  bool connect(const String& ssid, const String& pass);
  void startPortal();

  void handleRoot();
  void handleSave();
  void handleRedirect();

  Mode _mode = Mode::AccessPoint;
  WebServer _server{80};
  DNSServer _dns;
  String _networkOptions;   // <option> z wynikami skanowania sieci
  uint32_t _restartAt = 0;  // millis() restartu po zapisaniu ustawień, 0 = brak
};
