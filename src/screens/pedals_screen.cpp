#include "pedals_screen.h"

#include "../lang.h"
#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//   PEDALY                 < ABS TC HB HI >    y 4..29
//   ┌──┐ ┌──┐ ┌──┐   ┌──────────────────┐
//   │  │ │  │ │  │   │  wykres gaz/ham. │      y 40..189
//   │██│ │  │ │  │   │  (~5 s historii) │
//   └──┘ └──┘ └──┘   └──────────────────┘
//   Gaz  Ham. Sprz.   gaz  hamulec             y 196..
//   75%   0%   0%
// ---------------------------------------------------------------------

static const int BAR_Y = 40, BAR_H = 150, BAR_W = 38;
static const int BAR_X[3] = {36, 80, 124};   // z lewej ikona konfiguracji
static const StrId BAR_LABEL[3] = {S_THROTTLE_SHORT, S_BRAKE_SHORT, S_CLUTCH_SHORT};
static const uint16_t BAR_COLOR[3] = {TFT_GREEN, TFT_RED, TFT_SKYBLUE};

static const int GRAPH_X = 166, GRAPH_Y = 40, GRAPH_W = 150, GRAPH_H = 150;

void PedalsScreen::enter() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(tr(S_PEDALS_TITLE), 6, 4, 4);

  _tft.setTextDatum(TC_DATUM);
  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  for (int i = 0; i < 3; i++) _tft.drawString(tr(BAR_LABEL[i]), BAR_X[i] + BAR_W / 2, BAR_Y + BAR_H + 6, 2);

  // legenda wykresu
  _tft.setTextDatum(TL_DATUM);
  _tft.setTextColor(TFT_GREEN, TFT_BLACK);
  _tft.drawString(tr(S_GRAPH_THROTTLE), GRAPH_X, GRAPH_Y + GRAPH_H + 6, 2);
  _tft.setTextColor(TFT_RED, TFT_BLACK);
  _tft.drawString(tr(S_GRAPH_BRAKE), GRAPH_X + 40, GRAPH_Y + GRAPH_H + 6, 2);
  _tft.drawRect(GRAPH_X - 1, GRAPH_Y - 1, GRAPH_W + 2, GRAPH_H + 2, ui::DIM);
  ui::drawConfigIcon(_tft);

  _graph.setColorDepth(8);
  _graph.createSprite(GRAPH_W, GRAPH_H);
  _graph.fillSprite(TFT_BLACK);
  _graph.setScrollRect(0, 0, GRAPH_W, GRAPH_H, TFT_BLACK);
  _prevThrottleY = _prevBrakeY = -1;

  for (int32_t& v : _shownBar) v = -1;
  _shownLights = 0xFFFFFFFF;
}

void PedalsScreen::leave() {
  _graph.deleteSprite();
}

void PedalsScreen::update(const Telemetry& t) {
  const float values[3] = {t.d.throttle, t.d.brake, t.d.clutch};

  for (int i = 0; i < 3; i++) {
    int32_t pct = constrain(lroundf(values[i] * 100), 0, 100);
    if (pct == _shownBar[i]) continue;
    _shownBar[i] = pct;

    ui::drawVBar(_tft, BAR_X[i], BAR_Y, BAR_W, BAR_H, pct * BAR_H / 100, BAR_COLOR[i]);
    _tft.setTextDatum(TC_DATUM);
    _tft.setTextColor(TFT_WHITE, TFT_BLACK);
    _tft.setTextPadding(BAR_W + 4);
    _tft.drawString(String(pct) + "%", BAR_X[i] + BAR_W / 2, BAR_Y + BAR_H + 24, 2);
  }

  uint32_t lights = t.d.lights & ui::lightsMask();
  if (lights != _shownLights) {
    _shownLights = lights;
    ui::drawLights(_tft, 318 - ui::lightsWidth(), 4, lights);
  }

  drawGraphColumn(t.d.throttle, t.d.brake);
  _graph.pushSprite(GRAPH_X, GRAPH_Y);
}

// Przesuwa wykres o 1 piksel w lewo i dorysowuje nową kolumnę z prawej.
void PedalsScreen::drawGraphColumn(float throttle, float brake) {
  const int x = GRAPH_W - 1;
  _graph.scroll(-1, 0);

  // linie pomocnicze 25/50/75%
  for (int q = 1; q < 4; q++) _graph.drawPixel(x, GRAPH_H - 1 - q * (GRAPH_H - 1) / 4, ui::DIM);

  int ty = GRAPH_H - 1 - lroundf(constrain(throttle, 0.0f, 1.0f) * (GRAPH_H - 1));
  int by = GRAPH_H - 1 - lroundf(constrain(brake, 0.0f, 1.0f) * (GRAPH_H - 1));
  if (_prevThrottleY < 0) _prevThrottleY = ty;
  if (_prevBrakeY < 0) _prevBrakeY = by;

  _graph.drawLine(x - 1, _prevThrottleY, x, ty, TFT_GREEN);
  _graph.drawLine(x - 1, _prevBrakeY, x, by, TFT_RED);
  _prevThrottleY = ty;
  _prevBrakeY = by;
}
