#include "lang.h"

#include <Arduino.h>
#include <Preferences.h>

static const char* PREFS_NAMESPACE = "lang";
static const char* PREFS_KEY = "lang";

static const uint8_t LANG_COUNT = (uint8_t)Lang::COUNT;

// [tekst][język] – kolumny w kolejności Lang
#define LANG_ROW(id, ...) {__VA_ARGS__},
static const char* const STRINGS[STR_COUNT][LANG_COUNT] = {LANG_STRINGS(LANG_ROW)};
#undef LANG_ROW

static Lang current_ = Lang::PL;

const char* tr(StrId id) {
  if (id >= STR_COUNT) return "";
  const char* s = STRINGS[id][(uint8_t)current_];
  return s ? s : STRINGS[id][0];   // brak tłumaczenia -> po polsku
}

namespace lang {

void begin() {
  Preferences prefs;
  if (prefs.begin(PREFS_NAMESPACE, true)) {   // namespace jeszcze nie istnieje -> domyślny PL
    uint8_t v = prefs.getUChar(PREFS_KEY, 0);
    current_ = v < LANG_COUNT ? (Lang)v : Lang::PL;
    prefs.end();
  }
}

Lang current() {
  return current_;
}

void next() {
  current_ = (Lang)(((uint8_t)current_ + 1) % LANG_COUNT);
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putUChar(PREFS_KEY, (uint8_t)current_);
  prefs.end();
  Serial.printf("[Jezyk] %u\n", (uint8_t)current_);
}

void clear() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}

}  // namespace lang
