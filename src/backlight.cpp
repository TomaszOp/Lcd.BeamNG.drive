#include "backlight.h"

#include <Preferences.h>

static const char* PREFS_NAMESPACE = "display";
static const char* PREFS_KEY = "bright";

void Backlight::begin() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, true)) {
    _level = prefs.getUChar(PREFS_KEY, 100);
    prefs.end();
  }
  set(_level);
}

void Backlight::set(uint8_t percent) {
  _level = constrain(percent, MIN_LEVEL, 100);
#if defined(TFT_BL) && TFT_BL >= 0
  uint8_t duty = (uint32_t)_level * 255 / 100;
  analogWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? duty : 255 - duty);
#endif
}

void Backlight::save() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar(PREFS_KEY, _level);
  prefs.end();
  Serial.printf("[Podswietlenie] Zapisano %u%%\n", _level);
}

void Backlight::clear() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}
