#include "truck_screen.h"

#include "../lang.h"
#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//   CIEZAROWKA                    [OIL] [BAT]    y 4..29
//   Tempomat        Limit                        4 wiersze po 2 komórki
//   85 km/h         90 km/h                      (opis 16 px + wartość 26 px)
//   Powietrze       AdBlue
//   8.2 bar         64%
//   Retarder        Zasieg
//   2/5             830 km
//   Do celu         Czas
//   123 km          2:15
//  ⚙  [HB] [MIJ] [DLUG] [HAM.S] [AWAR]          y 212..233
// ---------------------------------------------------------------------

static const StrId LABELS[] = {S_CRUISE, S_LIMIT, S_AIR, S_ADBLUE, S_RETARDER, S_RANGE, S_NAV_DISTANCE, S_NAV_TIME};
static const int COL_X[2] = {36, 180};          // z lewej ikona konfiguracji
static const int ROW_Y = 36, ROW_STEP = 43, VALUE_DY = 16, CELL_W = 136;

static const float AIR_LOW_BAR = 5.0f;          // poniżej – kolor ostrzegawczy (gdy gra nie zgłasza)
static const float ADBLUE_LOW = 0.15f;

struct LampDef {
  StrId label;
  uint32_t bit;
  uint8_t width;
  uint16_t color;
};
static const LampDef LAMPS[] = {
    {S_LAMP_HANDBRAKE, L_HANDBRAKE, 34, TFT_RED},
    {S_LAMP_LOW_BEAM, L_LOW_BEAM, 40, TFT_GREEN},
    {S_LAMP_HIGH_BEAM, L_HIGH_BEAM, 46, TFT_BLUE},
    {S_LAMP_ENGINE_BRAKE, L_ENGINE_BRAKE, 52, TFT_ORANGE},
    {S_LAMP_HAZARD, L_HAZARD, 50, TFT_ORANGE},
};
static const int LAMPS_X = 50, LAMPS_Y = 212, LAMP_GAP = 4;
static const int WARN_LAMP_W = 40, WARN_LAMP_Y = 5;

static String kmh(float v) {
  if (isnan(v)) return "--";
  if (v < 1) return tr(S_OFF);
  return String(lroundf(v)) + " km/h";
}

static String duration(float minutes) {
  if (isnan(minutes) || minutes <= 0) return "--";
  int m = lroundf(minutes);
  char buf[12];
  snprintf(buf, sizeof(buf), "%d:%02d", m / 60, m % 60);
  return buf;
}

void TruckScreen::enter() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(tr(S_TRUCK_TITLE), 6, 4, 4);

  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  for (int i = 0; i < CELL_COUNT; i++)
    _tft.drawString(tr(LABELS[i]), COL_X[i % 2], ROW_Y + (i / 2) * ROW_STEP, 2);
  ui::drawConfigIcon(_tft);

  for (int i = 0; i < CELL_COUNT; i++) {
    _shownText[i] = "";
    _shownColor[i] = 0;
  }
  _shownLamps = 0xFFFFFFFF;
}

void TruckScreen::update(const Telemetry& t) {
  const CarData& d = t.d;
  uint32_t lights = d.lights;

  // tempomat / limit – przekroczenie limitu na czerwono
  drawCell(0, kmh(d.cruiseSpeed), d.cruiseSpeed >= 1 ? TFT_GREEN : TFT_WHITE);
  bool overLimit = d.speedLimit >= 1 && d.speed > d.speedLimit + 2;
  drawCell(1, kmh(d.speedLimit), overLimit ? TFT_RED : TFT_WHITE);

  // powietrze – hamowanie awaryjne miga na czerwono
  bool airEmerg = lights & L_AIR_EMERG;
  bool airWarn = (lights & L_AIR_WARN) || d.airPressure < AIR_LOW_BAR;
  uint16_t airColor = airEmerg ? (t.blinkOn ? TFT_RED : TFT_BLACK) : airWarn ? TFT_ORANGE : TFT_WHITE;
  drawCell(2, ui::valueText(d.airPressure, 1, " bar"), airColor);

  bool adblueWarn = (lights & L_ADBLUE_WARN) || d.adblue < ADBLUE_LOW;
  drawCell(3, ui::valueText(d.adblue * 100, 0, "%"), adblueWarn ? TFT_ORANGE : TFT_WHITE);

  String retarder = d.retarderSteps == 0 ? String(tr(S_NONE))
                    : d.retarder == 0    ? String(tr(S_OFF))
                                         : String(d.retarder) + "/" + String(d.retarderSteps);
  drawCell(4, retarder, d.retarder ? TFT_GREEN : TFT_WHITE);

  drawCell(5, ui::valueText(d.fuelRange, 0, " km"), (lights & L_FUEL_WARN) ? TFT_ORANGE : TFT_WHITE);
  drawCell(6, d.navDistance > 0 ? ui::valueText(d.navDistance, 0, " km") : String("--"), TFT_WHITE);
  drawCell(7, duration(d.navTime), TFT_WHITE);

  // kontrolki: dolny rząd + OIL/BAT w rogu (miganie kierunkowskazów na ekranie 1)
  uint32_t lamps = lights & (L_HANDBRAKE | L_LOW_BEAM | L_HIGH_BEAM | L_ENGINE_BRAKE | L_HAZARD | L_OIL_WARN | L_BATTERY);
  if (lamps != _shownLamps) {
    _shownLamps = lamps;
    int x = LAMPS_X;
    for (const LampDef& l : LAMPS) {
      ui::drawLamp(_tft, x, LAMPS_Y, l.width, tr(l.label), lamps & l.bit, l.color);
      x += l.width + LAMP_GAP;
    }
    ui::drawLamp(_tft, 318 - 2 * WARN_LAMP_W - 4, WARN_LAMP_Y, WARN_LAMP_W, "OIL", lamps & L_OIL_WARN, TFT_RED);
    ui::drawLamp(_tft, 318 - WARN_LAMP_W, WARN_LAMP_Y, WARN_LAMP_W, "BAT", lamps & L_BATTERY, TFT_RED);
  }
}

void TruckScreen::drawCell(int index, const String& text, uint16_t color) {
  if (text == _shownText[index] && color == _shownColor[index]) return;
  _shownText[index] = text;
  _shownColor[index] = color;
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(color, TFT_BLACK);
  _tft.setTextPadding(CELL_W);
  _tft.drawString(text, COL_X[index % 2], ROW_Y + (index / 2) * ROW_STEP + VALUE_DY, 4);
}
