// Gry (BeamNG.drive OutGauge, ETS2/ATS wtyczka ETS2Mod) -> UDP -> ESP32 -> ILI9341 + dioda RGB
// Obsługiwane gry i ich ekrany: games/game_list.cpp
#include <Arduino.h>
#include <TFT_eSPI.h>

#include "backlight.h"
#include "dashboard.h"
#include "games/game_receiver.h"
#include "in_receiver.h"
#include "lang.h"
#include "ota.h"
#include "rgb_led.h"
#include "touch.h"
#include "wifi_setup.h"

TFT_eSPI tft;
RgbLed led;
Touch touch(tft);
Backlight backlight;
Dashboard dashboard(tft, led, touch, backlight);
GameReceiver receiver(OUTGAUGE_PORT, dashboard);
InReceiver inputs(dashboard);
WifiSetup wifi;
Ota ota(dashboard);

void setup() {
  Serial.begin(115200);
  lang::begin();   // język tekstów z NVS – przed pierwszym rysowaniem
  led.begin();
  dashboard.begin();
  backlight.begin();   // jasność z NVS (po tft.init, który ustawia pin podświetlenia)
  touch.begin();   // brak kalibracji -> kalibracja i test, nieudana -> dotyk wyłączony
  inputs.begin();

  String saved = WifiSetup::savedSsid();
  if (saved.length()) dashboard.showConnecting(saved);
  if (wifi.begin() == WifiSetup::Mode::Station) {
    receiver.begin();
    ota.begin();   // tylko w sieci, nie w trybie hotspot (puste przy OTA_ENABLE 0)
    dashboard.showWaiting(wifi.ip().toString(), receiver.port());
  } else {
    dashboard.showPortal(wifi.ssid(), WIFI_AP_PASSWORD, wifi.ip().toString());
  }
}

void loop() {
  wifi.loop();
  if (wifi.mode() == WifiSetup::Mode::Station) {
    receiver.loop();
    ota.loop();
  }
  inputs.loop();
  dashboard.loop();
}
