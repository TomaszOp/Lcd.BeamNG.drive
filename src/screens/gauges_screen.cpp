#include "gauges_screen.h"

#include "../lang.h"
#include "widgets.h"

// ---------------------------------------------------------------------
//  Układ ekranu 320x240
//
//            < ABS TC HB HI >                 y 4..25
//    ╭──────────╮      ╭──────────╮
//   │  km/h      │    │   x1000    │          y 32..191
//   │     ●      │    │     ●      │
//   │    123     │    │     3      │
//    ╰──────────╯      ╰──────────╯
//   Gaz [=======]      Hamulec [====]         y 198..229
// ---------------------------------------------------------------------

static const int DIAL = 160;            // rozmiar sprite'a zegara
static const int C = DIAL / 2;          // środek
static const int R = 78;                // promień tarczy
static const int SPEED_X = 0, RPM_X = 160, DIAL_Y = 32;

static const float START_DEG = -120;    // 0 na skali (od godziny 12, zgodnie z zegarem)
static const float SWEEP_DEG = 240;     // zakres skali

// Skala prędkościomierza: OutGauge nie podaje Vmax auta, więc skala zaczyna od 160
// i przeskakuje na następny stopień, gdy prędkość przekroczy SPEED_SWITCH skali.
// Wraca do pierwszego stopnia po zmianie auta / przerwie w danych (topSpeed od Dashboard).
// Stopnie skali są w GaugesScreen::speedScale().
static const float SPEED_SWITCH = 0.95f;
static const float NEEDLE_EASE = 0.4f;  // jaką część drogi do celu wskazówka robi na klatkę

static const int LIGHTS_Y = 4;
static const int BAR_Y = 218, BAR_H = 12, BAR_W = 136, THROTTLE_X = 36, BRAKE_X = 180;   // z lewej ikona konfiguracji

// punkt na tarczy: frac 0..1 wzdłuż skali, r = odległość od środka
static void polar(float frac, float r, int& x, int& y) {
  float a = (START_DEG + SWEEP_DEG * frac) * DEG_TO_RAD;
  x = C + lroundf(r * sinf(a));
  y = C - lroundf(r * cosf(a));
}

// kąt dla drawArc (0 = godzina 6, zgodnie z zegarem)
static uint32_t arcAngle(float frac) {
  return lroundf(180 + START_DEG + SWEEP_DEG * frac);
}

void GaugesScreen::enter() {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextColor(ui::LABEL, TFT_BLACK);
  _tft.setTextDatum(TL_DATUM);
  _tft.drawString(tr(S_THROTTLE), THROTTLE_X, BAR_Y - 18, 2);
  _tft.drawString(tr(S_BRAKE), BRAKE_X, BAR_Y - 18, 2);
  ui::drawConfigIcon(_tft);

  // 8 bitów na piksel – połowa pamięci, wystarczy na tarczę i wskazówkę
  for (TFT_eSprite* s : {&_speedDial, &_rpmDial}) {
    s->setColorDepth(8);
    s->createSprite(DIAL, DIAL);
  }

  _shownSpeedMax = -1;
  _shownSpeedNeedle = _shownSpeed = _shownRpmNeedle = _shownRpmKey = -1;
  _shownThrottle = _shownBrake = -1;
  _shownLights = 0xFFFFFFFF;
}

void GaugesScreen::leave() {
  _speedDial.deleteSprite();
  _rpmDial.deleteSprite();
}

void GaugesScreen::update(const Telemetry& t) {
  drawSpeedDial(t);
  drawRpmDial(t);

  uint32_t lights = t.d.lights & ui::lightsMask();
  if (lights != _shownLights) {
    _shownLights = lights;
    ui::drawLights(_tft, (320 - ui::lightsWidth()) / 2, LIGHTS_Y, lights);
  }

  int32_t throttle = lroundf(t.d.throttle * BAR_W);
  if (throttle != _shownThrottle) {
    _shownThrottle = throttle;
    ui::drawHBar(_tft, THROTTLE_X, BAR_Y, BAR_W, BAR_H, throttle, TFT_GREEN);
  }
  int32_t brake = lroundf(t.d.brake * BAR_W);
  if (brake != _shownBrake) {
    _shownBrake = brake;
    ui::drawHBar(_tft, BRAKE_X, BAR_Y, BAR_W, BAR_H, brake, TFT_RED);
  }
}

// ---------------------------------------------------------------------

const GaugesScreen::DialScale& GaugesScreen::speedScale(const Telemetry& t) {
  // ciężarówki (OPT_SPEED_140): stała skala, ogranicznik i tak trzyma prędkość
  static const DialScale TRUCK = {140, 10, 20, 20, 1, 1.0f};
  if (t.game.options & OPT_SPEED_140) return TRUCK;

  //                                  max, krótka, długa, liczba, dzielnik, czerwone (1 = brak)
  static const DialScale SCALES[] = {{160, 10, 20, 20, 1, 1.0f},
                                     {200, 10, 20, 40, 1, 1.0f},
                                     {240, 10, 20, 40, 1, 1.0f},
                                     {300, 10, 50, 50, 1, 1.0f},
                                     {360, 20, 40, 60, 1, 1.0f},
                                     {400, 20, 40, 80, 1, 1.0f}};
  static const int COUNT = sizeof(SCALES) / sizeof(SCALES[0]);
  for (int i = 0; i < COUNT - 1; i++)
    if (t.topSpeed <= SCALES[i].maxValue * SPEED_SWITCH) return SCALES[i];
  return SCALES[COUNT - 1];
}

void GaugesScreen::drawSpeedDial(const Telemetry& t) {
  const DialScale& scale = speedScale(t);
  float kmh = t.d.speed;
  _speedNeedle += (constrain(kmh, 0.0f, scale.maxValue * 1.04f) - _speedNeedle) * NEEDLE_EASE;
  if (!isfinite(_speedNeedle)) _speedNeedle = 0;   // NaN zostałby we wskazówce na stałe

  int32_t needleKey = lroundf(_speedNeedle * 2);   // co 0.5 km/h
  int32_t speed = lroundf(kmh);
  if (needleKey == _shownSpeedNeedle && speed == _shownSpeed && scale.maxValue == _shownSpeedMax) return;
  _shownSpeedNeedle = needleKey;
  _shownSpeed = speed;
  _shownSpeedMax = scale.maxValue;

  drawFace(_speedDial, scale);

  _speedDial.setTextDatum(MC_DATUM);
  _speedDial.setTextColor(ui::LABEL);
  _speedDial.drawString("km/h", C, C - 30, 2);
  _speedDial.setTextColor(TFT_WHITE);
  _speedDial.drawNumber(speed, C, C + 36, 4);

  drawNeedle(_speedDial, _speedNeedle / scale.maxValue);
  _speedDial.pushSprite(SPEED_X, DIAL_Y);
}

void GaugesScreen::drawRpmDial(const Telemetry& t) {
  // skala do pełnych tysięcy powyżej najwyższych obrotów auta
  float maxValue = ceilf(t.maxRpm / 1000.0f) * 1000.0f;
  _rpmNeedle += (constrain(t.d.rpm, 0.0f, maxValue * 1.04f) - _rpmNeedle) * NEEDLE_EASE;
  if (!isfinite(_rpmNeedle)) _rpmNeedle = 0;

  bool flash = t.shift && t.blinkOn;
  int32_t needleKey = lroundf(_rpmNeedle / 20);    // co 20 rpm
  int32_t key = (int32_t)(maxValue / 1000) * 1000 + t.d.gear * 2 + flash;
  if (needleKey == _shownRpmNeedle && key == _shownRpmKey) return;
  _shownRpmNeedle = needleKey;
  _shownRpmKey = key;

  DialScale scale = {maxValue, 500, 1000, 1000, 1000, 0.85f};
  drawFace(_rpmDial, scale);
  if (flash) _rpmDial.drawArc(C, C, R, R - 5, arcAngle(0), arcAngle(1), TFT_RED, TFT_BLACK, false);

  _rpmDial.setTextDatum(MC_DATUM);
  _rpmDial.setTextColor(ui::LABEL);
  _rpmDial.drawString("x1000", C, C - 30, 2);

  _rpmDial.setFreeFont(&FreeSansBold24pt7b);
  _rpmDial.setTextColor(t.shift ? TFT_RED : ui::gearColor(t.d.gear));
  _rpmDial.drawString(ui::gearText(t.d.gear), C, C + 40);
  _rpmDial.setTextFont(1);

  drawNeedle(_rpmDial, _rpmNeedle / maxValue);
  _rpmDial.pushSprite(RPM_X, DIAL_Y);
}

void GaugesScreen::drawFace(TFT_eSprite& s, const DialScale& scale) {
  s.fillSprite(TFT_BLACK);
  s.drawCircle(C, C, R, ui::DIM);
  if (scale.redFrom < 1.0f)
    s.drawArc(C, C, R - 2, R - 9, arcAngle(scale.redFrom), arcAngle(1), TFT_RED, TFT_BLACK, false);

  int steps = lroundf(scale.maxValue / scale.minorStep);
  int majorEvery = lroundf(scale.majorStep / scale.minorStep);
  int labelEvery = lroundf(scale.labelStep / scale.minorStep);

  s.setTextDatum(MC_DATUM);
  s.setTextColor(TFT_WHITE);
  for (int i = 0; i <= steps; i++) {
    float frac = (float)i / steps;
    bool major = i % majorEvery == 0;
    int x1, y1, x2, y2;
    polar(frac, R - 2, x1, y1);
    polar(frac, major ? R - 14 : R - 8, x2, y2);
    s.drawLine(x1, y1, x2, y2, major ? TFT_WHITE : TFT_DARKGREY);

    if (i % labelEvery == 0) {
      int lx, ly;
      polar(frac, R - 26, lx, ly);
      s.drawNumber(lroundf(i * scale.minorStep / scale.labelDiv), lx, ly, 2);
    }
  }
}

void GaugesScreen::drawNeedle(TFT_eSprite& s, float frac) {
  float a = (START_DEG + SWEEP_DEG * constrain(frac, 0.0f, 1.04f)) * DEG_TO_RAD;
  float sx = sinf(a), cy = cosf(a);

  int tipX = C + lroundf((R - 10) * sx), tipY = C - lroundf((R - 10) * cy);
  int tailX = C - lroundf(12 * sx), tailY = C + lroundf(12 * cy);
  int px = lroundf(4 * cy), py = lroundf(4 * sx);   // prostopadle do wskazówki

  s.fillTriangle(tailX + px, tailY + py, tailX - px, tailY - py, tipX, tipY, TFT_ORANGE);
  s.fillCircle(C, C, 7, TFT_DARKGREY);
  s.fillCircle(C, C, 3, TFT_BLACK);
}
