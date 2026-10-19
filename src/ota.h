// Wgrywanie programu przez WiFi (ArduinoOTA), włączane flagą OTA_ENABLE w config.h.
// Działa tylko po połączeniu z siecią (nie w trybie hotspot). Wgranie wymaga hasła
// OTA_PASSWORD, a urządzenie ogłasza się w sieci jako OTA_HOSTNAME.local (mDNS).
// Postęp pokazuje Dashboard; po wgraniu (albo błędzie) urządzenie się restartuje.
#pragma once

#include <Arduino.h>

class Dashboard;

class Ota {
public:
  explicit Ota(Dashboard& dashboard) : _dashboard(dashboard) {}

  void begin();   // wywołać po połączeniu z siecią
  void loop();

private:
  Dashboard& _dashboard;
  bool _started = false;
};
