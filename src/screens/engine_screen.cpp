#include "engine_screen.h"

#include "../lang.h"
#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//   SILNIK                        [OIL] [BAT]   y 4..29
//   Paliwo
//   [==============          ]         58%      5 wierszy co 40 px
//   Temp. silnika
//   [===========             ]         92C
//   ...
// ---------------------------------------------------------------------

struct RowDef {
  StrId label;
  float minValue, maxValue;   // zakres paska
  float warn, alarm;          // progi ostrzeżeń (NAN = brak)
  bool lowIsBad;              // ostrzeżenie poniżej progu zamiast powyżej
  uint8_t decimals;
  const char* unit;
};

static const RowDef ROWS[] = {
    {S_FUEL, 0, 100, 25, 12, true, 0, "%"},
    {S_ENGINE_TEMP, 40, 130, 100, 110, false, 0, "C"},
    {S_OIL_TEMP, 40, 150, 120, 135, false, 0, "C"},
    {S_OIL_PRESSURE, 0, 8, NAN, NAN, false, 1, " bar"},
    {S_TURBO, 0, 2.5, NAN, NAN, false, 1, " bar"},
};

static const int ROW_Y = 38, ROW_STEP = 40;
static const int BAR_X = 36, BAR_W = 168, BAR_H = 12, BAR_DY = 19;   // z lewej ikona konfiguracji
static const int VALUE_RIGHT = 316, VALUE_DY = 8;
static const int LAMP_W = 40, LAMP_Y = 5;

void EngineScreen::enter() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(tr(S_ENGINE_TITLE), 6, 4, 4);

  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  for (int i = 0; i < ROW_COUNT; i++) _tft.drawString(tr(ROWS[i].label), BAR_X, ROW_Y + i * ROW_STEP, 2);
  ui::drawConfigIcon(_tft);

  for (int32_t& k : _shownKey) k = INT32_MIN;
  _shownLamps = 0xFFFFFFFF;
}

void EngineScreen::update(const Telemetry& t) {
  const CarData& d = t.d;

  // kontrolki ostrzegawcze
  uint32_t lamps = d.lights & (L_OIL_WARN | L_BATTERY);
  if (lamps != _shownLamps) {
    _shownLamps = lamps;
    ui::drawLamp(_tft, 318 - 2 * LAMP_W - 4, LAMP_Y, LAMP_W, "OIL", lamps & L_OIL_WARN, TFT_RED);
    ui::drawLamp(_tft, 318 - LAMP_W, LAMP_Y, LAMP_W, "BAT", lamps & L_BATTERY, TFT_RED);
  }

  const float values[ROW_COUNT] = {d.fuel * 100.0f, d.engTemp, d.oilTemp, d.oilPressure, d.turbo};

  for (int i = 0; i < ROW_COUNT; i++) {
    const RowDef& row = ROWS[i];
    float v = values[i];

    if (isnan(v)) {   // gra nie podaje tej wartości
      if (_shownKey[i] == INT32_MAX) continue;
      _shownKey[i] = INT32_MAX;
      int y = ROW_Y + i * ROW_STEP;
      ui::drawHBar(_tft, BAR_X, y + BAR_DY, BAR_W, BAR_H, 0, TFT_GREEN);
      _tft.setTextDatum(TR_DATUM);
      _tft.setTextColor(ui::LABEL, TFT_BLACK);
      _tft.setTextPadding(_tft.textWidth("88.8 bar", 4));
      _tft.drawString("--", VALUE_RIGHT, y + VALUE_DY, 4);
      continue;
    }

    bool alarm = !isnan(row.alarm) && (row.lowIsBad ? v < row.alarm : v > row.alarm);
    bool warn = !isnan(row.warn) && (row.lowIsBad ? v < row.warn : v > row.warn);
    bool hidden = alarm && !t.blinkOn;   // alarm miga

    int32_t key = lroundf(v * 10) * 4 + (alarm ? 2 : warn ? 1 : 0) + (hidden ? 100000 : 0);
    if (key == _shownKey[i]) continue;
    _shownKey[i] = key;

    uint16_t color = alarm ? TFT_RED : warn ? TFT_ORANGE : TFT_GREEN;
    int y = ROW_Y + i * ROW_STEP;

    float frac = (v - row.minValue) / (row.maxValue - row.minValue);
    ui::drawHBar(_tft, BAR_X, y + BAR_DY, BAR_W, BAR_H, lroundf(frac * BAR_W), color);

    char buf[16];
    snprintf(buf, sizeof(buf), "%.*f%s", row.decimals, v, row.unit);
    _tft.setTextDatum(TR_DATUM);
    _tft.setTextColor(hidden ? TFT_BLACK : (alarm || warn ? color : TFT_WHITE), TFT_BLACK);
    _tft.setTextPadding(_tft.textWidth("88.8 bar", 4));
    _tft.drawString(buf, VALUE_RIGHT, y + VALUE_DY, 4);
  }
}
