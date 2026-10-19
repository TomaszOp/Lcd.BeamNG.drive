#include "widgets.h"

#include "../games/car_data.h"

namespace ui {

struct LightDef {
  const char* label;
  uint32_t bit;
  uint8_t width;
  uint16_t color;
};
static const LightDef LIGHTS[] = {
    {"<", L_SIGNAL_L, 22, TFT_GREEN},
    {"ABS", L_ABS, 36, TFT_ORANGE},
    {"TC", L_TC, 30, TFT_ORANGE},
    {"HB", L_HANDBRAKE, 30, TFT_RED},
    {"HI", L_HIGH_BEAM, 30, TFT_BLUE},
    {">", L_SIGNAL_R, 22, TFT_GREEN},
};
static const int LIGHTS_H = 22, LIGHTS_GAP = 3;

int lightsWidth() {
  int total = -LIGHTS_GAP;
  for (const LightDef& l : LIGHTS) total += l.width + LIGHTS_GAP;
  return total;
}

uint32_t lightsMask() {
  uint32_t mask = 0;
  for (const LightDef& l : LIGHTS) mask |= l.bit;
  return mask;
}

void drawLights(TFT_eSPI& gfx, int x, int y, uint32_t lights) {
  for (const LightDef& l : LIGHTS) {
    drawLamp(gfx, x, y, l.width, l.label, lights & l.bit, l.color);
    x += l.width + LIGHTS_GAP;
  }
}

void drawLamp(TFT_eSPI& gfx, int x, int y, int w, const char* label, bool on, uint16_t color) {
  gfx.fillRoundRect(x, y, w, LIGHTS_H, 4, on ? color : TFT_BLACK);
  gfx.drawRoundRect(x, y, w, LIGHTS_H, 4, on ? color : DIM);
  gfx.setTextDatum(MC_DATUM);
  gfx.setTextPadding(0);
  gfx.setTextColor(on ? TFT_BLACK : DIM);
  gfx.drawString(label, x + w / 2, y + LIGHTS_H / 2 + 1, 2);
}

void drawHBar(TFT_eSPI& gfx, int x, int y, int w, int h, int filled, uint16_t color) {
  filled = constrain(filled, 0, w);
  if (filled > 0) gfx.fillRect(x, y, filled, h, color);
  if (filled < w) gfx.fillRect(x + filled, y, w - filled, h, DIM);
}

void drawVBar(TFT_eSPI& gfx, int x, int y, int w, int h, int filled, uint16_t color) {
  filled = constrain(filled, 0, h);
  if (filled < h) gfx.fillRect(x, y, w, h - filled, DIM);
  if (filled > 0) gfx.fillRect(x, y + h - filled, w, filled, color);
}

void drawConfigIcon(TFT_eSPI& gfx) {
  const int cx = CONFIG_ICON_X, cy = CONFIG_ICON_Y;
  for (int k = 0; k < 8; k++) {   // zęby
    float a = k * PI / 4;
    gfx.fillCircle(cx + lroundf(9 * cosf(a)), cy + lroundf(9 * sinf(a)), 3, LABEL);
  }
  gfx.fillCircle(cx, cy, 8, LABEL);
  gfx.fillCircle(cx, cy, 3, TFT_BLACK);
}

bool configIconHit(uint16_t x, uint16_t y) {
  return x < 50 && y > 190;
}

String gearText(int8_t gear) {
  if (gear == 0) return "N";
  if (gear == -1) return "R";
  if (gear < 0) return "R" + String(-gear);
  return String(gear);
}

uint16_t gearColor(int8_t gear) {
  if (gear < 0) return TFT_RED;
  if (gear == 0) return TFT_GREEN;
  return TFT_WHITE;
}

String valueText(float value, int decimals, const char* unit) {
  if (isnan(value)) return "--";
  return String(value, (unsigned int)decimals) + unit;
}

}  // namespace ui
