#include "ota.h"

#include "dashboard.h"

#if OTA_ENABLE

#include <ArduinoOTA.h>

void Ota::begin() {
  ArduinoOTA.setHostname(OTA_HOSTNAME);
  ArduinoOTA.setPassword(OTA_PASSWORD);

  ArduinoOTA.onStart([this] {
    Serial.println("[OTA] Start");
    _dashboard.showOtaStart();
  });
  ArduinoOTA.onProgress([this](unsigned int done, unsigned int total) {
    _dashboard.showOtaProgress(total ? done * 100 / total : 0);
  });
  ArduinoOTA.onEnd([] { Serial.println("[OTA] Koniec, restart"); });   // restart robi ArduinoOTA
  ArduinoOTA.onError([this](ota_error_t error) {
    Serial.printf("[OTA] Blad %u\n", error);
    _dashboard.showOtaError();
    delay(2000);    // chwila, żeby przeczytać komunikat
    ESP.restart();  // najprościej wrócić do normalnej pracy
  });

  ArduinoOTA.begin();
  _started = true;
  Serial.printf("[OTA] Gotowe: %s.local, port 3232\n", OTA_HOSTNAME);
}

void Ota::loop() {
  // podczas wgrywania handle() blokuje do końca transferu
  if (_started) ArduinoOTA.handle();
}

#else

void Ota::begin() {}
void Ota::loop() {}

#endif
