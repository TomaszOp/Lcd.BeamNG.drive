#include "race_screen.h"

#include <string.h>

#include "../lang.h"
#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//   [============ pasek obrotów ============]   y 0..29
//   6500 rpm            < ABS TC HB HI >        y 36..57
//   SPEED  |   GEAR   |  Paliwo / temp / turbo  y 66..175
//   Gaz      [=========                ]        y 186..
//   Hamulec  [===                      ]
//   Sprzeglo [                         ]
// ---------------------------------------------------------------------

// pasek obrotów
static const int RPM_BAR_H = 30;
static const int RPM_SEGMENTS = 20;
static const int RPM_SEG_W = 320 / RPM_SEGMENTS;

// bieg
static const int GEAR_X = 110, GEAR_Y = 66, GEAR_W = 100, GEAR_H = 110;

// prędkość
static const int SPEED_RIGHT = 104, SPEED_Y = 82;

// kolumna z informacjami
static const int INFO_LABEL_X = 216, INFO_VALUE_RIGHT = 318, INFO_Y = 72, INFO_STEP = 26;

// pedały
static const int PEDAL_LABEL_X = 34, PEDAL_X = 76, PEDAL_W = 240, PEDAL_H = 12;   // z lewej ikona konfiguracji
static const int PEDAL_Y = 188, PEDAL_STEP = 18;

// kontrolki
static const int LIGHTS_RIGHT = 318, LIGHTS_Y = 36;

void RaceScreen::enter() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextColor(ui::LABEL, TFT_BLACK);

  _tft.setTextDatum(TL_DATUM);
  _tft.drawString("rpm", 100, 44, 2);

  _tft.setTextDatum(TR_DATUM);
  _tft.drawString("km/h", SPEED_RIGHT, SPEED_Y + 54, 2);

  _tft.setTextDatum(TL_DATUM);
  static const StrId infoLabels[] = {S_FUEL, S_ENGINE, S_OIL, S_TURBO};
  for (int i = 0; i < 4; i++) _tft.drawString(tr(infoLabels[i]), INFO_LABEL_X, INFO_Y + i * INFO_STEP, 2);

  static const StrId pedalLabels[] = {S_THROTTLE_SHORT, S_BRAKE_SHORT, S_CLUTCH_SHORT};
  for (int i = 0; i < 3; i++) _tft.drawString(tr(pedalLabels[i]), PEDAL_LABEL_X, PEDAL_Y + i * PEDAL_STEP - 2, 2);

  ui::drawConfigIcon(_tft);

  _rpmSprite.createSprite(320, RPM_BAR_H);
  _gearSprite.createSprite(GEAR_W, GEAR_H);
  _gearSprite.setFreeFont(&FreeSansBold24pt7b);
  _gearSprite.setTextSize(2);
  _gearSprite.setTextDatum(MC_DATUM);

  memset(&_shown, 0xFF, sizeof(_shown));   // wszystko do przerysowania
}

void RaceScreen::leave() {
  _rpmSprite.deleteSprite();
  _gearSprite.deleteSprite();
}

void RaceScreen::update(const Telemetry& t) {
  const CarData& d = t.d;

  drawRpmBar(t);
  drawGear(t);

  uint32_t lights = d.lights & ui::lightsMask();
  if (lights != _shown.lights) {
    _shown.lights = lights;
    ui::drawLights(_tft, LIGHTS_RIGHT - ui::lightsWidth(), LIGHTS_Y, lights);
  }

  // obroty
  int32_t rpm = lroundf(d.rpm / 10.0f) * 10;   // co 10, żeby cyfry nie migały
  if (rpm != _shown.rpm) {
    _shown.rpm = rpm;
    _tft.setTextDatum(TR_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextPadding(_tft.textWidth("88888", 4));
    _tft.drawNumber(rpm, 95, 38, 4);
  }

  // prędkość
  int32_t speed = lroundf(d.speed);
  if (speed != _shown.speed) {
    _shown.speed = speed;
    _tft.setTextDatum(TR_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextPadding(_tft.textWidth("888", 7));
    _tft.drawNumber(speed, SPEED_RIGHT, SPEED_Y, 7);
  }

  // informacje
  // klucz zmiany: wartość zaokrąglona do wyświetlanej precyzji, NAN = INT32_MAX ("--")
  auto key = [](float v, float scale) { return isnan(v) ? INT32_MAX : (int32_t)lroundf(v * scale); };
  drawInfoValue(0, key(d.fuel, 100), _shown.fuel, ui::valueText(d.fuel * 100, 0, "%").c_str());
  drawInfoValue(1, key(d.engTemp, 1), _shown.engTemp, ui::valueText(d.engTemp, 0, "C").c_str());
  drawInfoValue(2, key(d.oilTemp, 1), _shown.oilTemp, ui::valueText(d.oilTemp, 0, "C").c_str());
  drawInfoValue(3, key(d.turbo, 10), _shown.turbo10, ui::valueText(d.turbo, 1, "").c_str());

  // pedały
  drawPedal(0, d.throttle, _shown.throttle, TFT_GREEN);
  drawPedal(1, d.brake, _shown.brake, TFT_RED);
  drawPedal(2, d.clutch, _shown.clutch, TFT_SKYBLUE);
}

void RaceScreen::drawRpmBar(const Telemetry& t) {
  int32_t lit = constrain(lroundf(t.rpmFrac * RPM_SEGMENTS), 0, RPM_SEGMENTS);
  int32_t key = t.shift ? (t.blinkOn ? 1000 : 1001) : lit;
  if (key == _shown.rpmBar) return;
  _shown.rpmBar = key;

  _rpmSprite.fillSprite(TFT_BLACK);
  for (int i = 0; i < RPM_SEGMENTS; i++) {
    uint16_t color;
    if (t.shift) color = t.blinkOn ? TFT_RED : ui::DIM;
    else if (i >= lit) color = ui::DIM;
    else if (i < 12) color = TFT_GREEN;
    else if (i < 16) color = ui::YELLOW;
    else color = TFT_RED;
    _rpmSprite.fillRoundRect(i * RPM_SEG_W + 1, 2, RPM_SEG_W - 2, RPM_BAR_H - 4, 3, color);
  }
  _rpmSprite.pushSprite(0, 0);
}

void RaceScreen::drawGear(const Telemetry& t) {
  bool flash = t.shift && t.blinkOn;
  int32_t key = (t.d.gear + 128) * 2 + flash;   // zawsze >= 0 (memset w enter() daje -1)
  if (key == _shown.gear) return;
  _shown.gear = key;

  uint16_t bg = flash ? TFT_RED : TFT_BLACK;
  uint16_t color = flash ? TFT_WHITE : ui::gearColor(t.d.gear);
  _gearSprite.fillSprite(bg);
  _gearSprite.drawRoundRect(0, 0, GEAR_W, GEAR_H, 8, ui::DIM);
  _gearSprite.setTextColor(color, bg);
  String text = ui::gearText(t.d.gear);
  _gearSprite.setTextSize(text.length() > 1 ? 1 : 2);   // "12", "R2" – dwa znaki nie zmieszczą się w rozmiarze 2
  _gearSprite.drawString(text, GEAR_W / 2, GEAR_H / 2);
  _gearSprite.pushSprite(GEAR_X, GEAR_Y);
}

void RaceScreen::drawInfoValue(int line, int32_t key, int32_t& shown, const char* text) {
  if (key == shown) return;
  shown = key;
  _tft.setTextDatum(TR_DATUM);
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.setTextPadding(_tft.textWidth("8888%", 2));
  _tft.drawString(text, INFO_VALUE_RIGHT, INFO_Y + line * INFO_STEP, 2);
}

void RaceScreen::drawPedal(int row, float value, int32_t& shown, uint16_t color) {
  int32_t w = constrain(lroundf(value * PEDAL_W), 0, PEDAL_W);
  if (w == shown) return;
  shown = w;
  ui::drawHBar(_tft, PEDAL_X, PEDAL_Y + row * PEDAL_STEP, PEDAL_W, PEDAL_H, w, color);
}
