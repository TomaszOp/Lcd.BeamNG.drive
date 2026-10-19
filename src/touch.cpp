#include "touch.h"

#include <Preferences.h>
#include <string.h>

#include "lang.h"
#include "screens/widgets.h"

static const char* PREFS_NAMESPACE = "touch";
static const char* PREFS_KEY = "cal";

static const int CORNER_MARGIN = 20;     // krzyżyk kalibracji tyle px od rogu
static const int TEST_MARGIN = 40;       // punkty testu nie bliżej krawędzi niż tyle px
static const int CROSS = 10;             // pół długości ramienia krzyżyka
static const uint16_t Z_CORNER = 175;    // próg nacisku przy kalibracji (rogi są mniej czułe)
static const uint16_t Z_TOUCH = 350;     // próg nacisku przy zwykłym odczycie
static const uint16_t Z_RELEASE = 100;   // poniżej tego nacisku uznajemy, że palec puścił
static const uint8_t CORNER_SAMPLES = 16;
static const uint8_t READ_SAMPLES = 4;

// Średnia ze środkowej połowy posortowanych próbek (sortuje tablicę).
static uint16_t middleMean(uint16_t* v, uint8_t n) {
  for (uint8_t i = 1; i < n; i++)
    for (uint8_t j = i; j > 0 && v[j - 1] > v[j]; j--) {
      uint16_t t = v[j];
      v[j] = v[j - 1];
      v[j - 1] = t;
    }
  uint32_t sum = 0;
  uint8_t from = n / 4, to = n - n / 4;
  for (uint8_t i = from; i < to; i++) sum += v[i];
  return sum / (to - from);
}

// ---------------------------------------------------------------------

void Touch::begin() {
#if !TOUCH_ENABLE
  _enabled = false;
  Serial.println("[Dotyk] Wylaczony (TOUCH_ENABLE 0)");
  return;
#endif
  if (PIN_TOUCH_IRQ >= 0) pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP);   // tylko do diagnostyki
  if (loadCalibration(_cal)) {
    _tft.setTouch(_cal);
    _enabled = true;
    Serial.println("[Dotyk] Kalibracja wczytana");
    return;
  }

  Serial.println("[Dotyk] Brak kalibracji - start kalibracji");
  _enabled = calibrateAndTest(_cal);
  if (!_enabled) {
    _tft.drawString(tr(S_TOUCH_OFF), 160, 100, 2);
    _tft.drawString(tr(S_CAL_NEXT_BOOT), 160, 124, 2);
    delay(2000);
  }
}

bool Touch::recalibrate() {
  Serial.println("[Dotyk] Kalibracja na zyczenie");
  uint16_t cal[CAL_SIZE];
  if (calibrateAndTest(cal)) {
    memcpy(_cal, cal, sizeof(_cal));
    _enabled = true;
    return true;
  }
  if (_enabled) _tft.setTouch(_cal);   // wracamy do poprzedniej kalibracji
  _tft.drawString(_enabled ? tr(S_CAL_KEPT) : tr(S_TOUCH_OFF), 160, 100, 2);
  delay(2000);
  return false;
}

// Kalibracja + test; udana -> zapis do NVS i ekran "Kalibracja OK" (2 s).
// Nieudana -> ekran "Kalibracja nieudana" (opis dopisuje wywołujący).
bool Touch::calibrateAndTest(uint16_t* data) {
  bool ok = calibrate(data) && runTest();
  title(ok ? tr(S_CAL_OK) : tr(S_CAL_FAILED));
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  Serial.printf("[Dotyk] %s\n", ok ? "Kalibracja zapisana" : "Kalibracja nieudana");
  if (ok) {
    saveCalibration(data);
    _tft.drawString(tr(S_CAL_SAVED), 160, 110, 2);
    delay(2000);
  }
  return ok;
}

bool Touch::read(uint16_t& x, uint16_t& y) {
  if (!_enabled) return false;
  return readPoint(x, y);
}

// Szybki odczyt zamiast TFT_eSPI::getTouch(), które robi 5 serii pomiarów i odrzuca
// dotyk przy różnicy próbek > 20 – na szumiącym panelu trzeba wtedy długo trzymać palec.
// Tu: nacisk z, kilka próbek uśrednionych, nacisk sprawdzony ponownie.
bool Touch::readRaw(uint16_t& rawX, uint16_t& rawY, uint16_t threshold) {
  if (!pressed(threshold)) return false;
  uint32_t sx = 0, sy = 0;
  for (uint8_t i = 0; i < READ_SAMPLES; i++) {
    uint16_t x, y;
    _tft.getTouchRaw(&x, &y);
    sx += x;
    sy += y;
  }
  if (!pressed(threshold)) return false;   // palec zszedł w trakcie
  rawX = sx / READ_SAMPLES;
  rawY = sy / READ_SAMPLES;
  return true;
}

bool Touch::readPoint(uint16_t& x, uint16_t& y) {
  uint16_t tx, ty;
  if (!readRaw(tx, ty, Z_TOUCH)) return false;
  _tft.convertRawXY(&tx, &ty);
  if (tx >= _tft.width() || ty >= _tft.height()) return false;
  x = tx;
  y = ty;
  return true;
}

// ---------------------------------------------------------------------
//  Pamięć NVS

bool Touch::loadCalibration(uint16_t* data) {
  Preferences prefs;
  if (!prefs.begin(PREFS_NAMESPACE, true)) return false;   // namespace jeszcze nie istnieje
  size_t len = prefs.isKey(PREFS_KEY) ? prefs.getBytes(PREFS_KEY, data, CAL_SIZE * sizeof(uint16_t)) : 0;
  prefs.end();
  return len == CAL_SIZE * sizeof(uint16_t);
}

void Touch::clearCalibration() {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.clear();
  prefs.end();
}

void Touch::saveCalibration(const uint16_t* data) {
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, false);
  prefs.putBytes(PREFS_KEY, data, CAL_SIZE * sizeof(uint16_t));
  prefs.end();
}

// ---------------------------------------------------------------------
//  Kalibracja – 4 rogi
//
//  Własna wersja TFT_eSPI::calibrateTouch(): tamta czeka na dotyk w nieskończoność
//  (bez podłączonego dotyku program by stanął), a tu każdy róg ma limit czasu.
//  Wynik w tym samym formacie co calibrateTouch(), ustawiany przez setTouch().

bool Touch::calibrate(uint16_t* data) {
  const int w = _tft.width(), h = _tft.height();
  const int l = CORNER_MARGIN, r = w - 1 - CORNER_MARGIN;
  const int t = CORNER_MARGIN, b = h - 1 - CORNER_MARGIN;

  // kolejność: lewy górny, lewy dolny, prawy górny, prawy dolny
  const int cx[4] = {l, l, r, r};
  const int cy[4] = {t, b, t, b};
  uint16_t rx[4], ry[4];

  title(tr(S_TOUCH_CALIBRATION));
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(S_CAL_HINT1), 160, 100, 2);
  _tft.drawString(tr(S_CAL_HINT2), 160, 120, 2);
  delay(2000);
  _tft.fillScreen(TFT_BLACK);

  for (int i = 0; i < 4; i++) {
    drawCross(cx[i], cy[i], TFT_WHITE);
    bool ok = readCorner(cx[i], cy[i], rx[i], ry[i]);
    drawCross(cx[i], cy[i], TFT_BLACK);
    if (!ok) {
      Serial.printf("[Dotyk] Brak dotyku w rogu %d\n", i + 1);
      return false;
    }
#if TOUCH_DEBUG_LOG
    Serial.printf("[Dotyk] Rog %d: raw %u, %u\n", i + 1, rx[i], ry[i]);
#endif
    waitRelease();
  }

  // Lewy górny -> lewy dolny zmienia y ekranu. Jeśli bardziej zmienia się surowe x,
  // osie dotyku i ekranu są zamienione (rotate).
  bool rotate = abs((int)rx[0] - (int)rx[1]) > abs((int)ry[0] - (int)ry[1]);
  const uint16_t* ax = rotate ? ry : rx;   // kanał surowy dla osi x ekranu
  const uint16_t* ay = rotate ? rx : ry;   // kanał surowy dla osi y ekranu

  // Surowe wartości przy lewej/prawej i górnej/dolnej linii krzyżyków
  float left = (ax[0] + ax[1]) / 2.0f, right = (ax[2] + ax[3]) / 2.0f;
  float top = (ay[0] + ay[2]) / 2.0f, bottom = (ay[1] + ay[3]) / 2.0f;
  if (fabsf(right - left) < 100 || fabsf(bottom - top) < 100) {
    Serial.println("[Dotyk] Za maly zakres odczytow - zla kalibracja");
    return false;
  }

  // Przeliczenie na krawędzie ekranu (krzyżyki są CORNER_MARGIN od krawędzi).
  // TFT_eSPI: ekran = (raw - x0) * szerokość / x1, przy odwróconej osi szerokość - wynik.
  float kx = (right - left) / (r - l), ky = (bottom - top) / (b - t);   // raw na piksel
  float x0 = left - l * kx, xw = x0 + w * kx;                           // raw przy x = 0 i x = w
  float y0 = top - t * ky, yh = y0 + h * ky;

  bool invertX = kx < 0, invertY = ky < 0;
  data[0] = (uint16_t)constrain(lroundf(invertX ? xw : x0), 1, 4095);
  data[1] = (uint16_t)constrain(lroundf(fabsf(w * kx)), 1, 4095);
  data[2] = (uint16_t)constrain(lroundf(invertY ? yh : y0), 1, 4095);
  data[3] = (uint16_t)constrain(lroundf(fabsf(h * ky)), 1, 4095);
  data[4] = (rotate ? 1 : 0) | (invertX ? 2 : 0) | (invertY ? 4 : 0);
  _tft.setTouch(data);

#if TOUCH_DEBUG_LOG
  Serial.printf("[Dotyk] Kalibracja: %u %u %u %u %u\n", data[0], data[1], data[2], data[3], data[4]);
#endif
  return true;
}

// Czeka na dotyk krzyżyka (limit TOUCH_CORNER_TIMEOUT_MS), zbiera CORNER_SAMPLES odczytów
// i bierze średnią ze środkowej połowy (odrzuca skrajne, zaszumione próbki).
bool Touch::readCorner(int px, int py, uint16_t& rawX, uint16_t& rawY) {
  uint32_t start = millis();
#if TOUCH_DEBUG_LOG
  uint32_t lastLog = 0;
#endif
  while (millis() - start < TOUCH_CORNER_TIMEOUT_MS) {
#if TOUCH_DEBUG_LOG
    // diagnostyka: co 0,5 s surowe odczyty (z = nacisk, próg Z_CORNER; IRQ: 0 = dotyk)
    if (millis() - lastLog >= 500) {
      lastLog = millis();
      uint16_t x, y;
      _tft.getTouchRaw(&x, &y);
      Serial.printf("[Dotyk] raw z=%u x=%u y=%u irq=%d\n", _tft.getTouchRawZ(), x, y,
                    PIN_TOUCH_IRQ >= 0 ? digitalRead(PIN_TOUCH_IRQ) : -1);
    }
#endif
    uint16_t xs[CORNER_SAMPLES], ys[CORNER_SAMPLES];
    uint8_t n = 0;
    uint32_t sampleStart = millis();
    while (n < CORNER_SAMPLES && millis() - sampleStart < 1000) {
      if (readRaw(xs[n], ys[n], Z_CORNER)) n++;
      delay(5);
    }
    if (n < CORNER_SAMPLES) {   // za krótki dotyk -> czekamy na kolejny
#if TOUCH_DEBUG_LOG
      if (n > 0) Serial.printf("[Dotyk] za krotki dotyk (%u probek)\n", n);
#endif
      continue;
    }

    rawX = middleMean(xs, n);
    rawY = middleMean(ys, n);
    drawCross(px, py, TFT_GREEN);
    delay(300);
    return true;
  }
  return false;
}

bool Touch::pressed(uint16_t threshold) {
  return _tft.getTouchRawZ() > threshold;
}

void Touch::waitRelease() {
  uint32_t releasedAt = 0;
  uint32_t start = millis();
  while (millis() - start < 3000) {   // nie czekamy w nieskończoność
    if (_tft.getTouchRawZ() > Z_RELEASE) {
      releasedAt = 0;
    } else if (!releasedAt) {
      releasedAt = millis();
    } else if (millis() - releasedAt > 150) {
      return;
    }
    delay(5);
  }
}

// ---------------------------------------------------------------------
//  Test – 4 losowe punkty, na każdy TOUCH_TEST_TIME_MS

bool Touch::runTest() {
  const int w = _tft.width(), h = _tft.height();

  title(tr(S_TOUCH_TEST));
  _tft.setTextColor(TFT_WHITE, TFT_BLACK);
  _tft.drawString(tr(S_TEST_HINT1), 160, 100, 2);
  _tft.drawString(tr(S_TEST_HINT2), 160, 120, 2);
  delay(2000);

  for (int i = 0; i < 4; i++) {
    int px = random(TEST_MARGIN, w - TEST_MARGIN);
    int py = random(TEST_MARGIN, h - TEST_MARGIN);

    _tft.fillScreen(TFT_BLACK);
    _tft.drawCircle(px, py, TOUCH_TEST_RADIUS, ui::DIM);
    drawCross(px, py, TFT_YELLOW);

    bool hit = false, touched = false;
    int shownSec = -1;
    uint32_t start = millis();
    while (millis() - start < TOUCH_TEST_TIME_MS) {
      // odliczanie u góry ekranu
      int sec = (TOUCH_TEST_TIME_MS - (millis() - start) + 999) / 1000;
      if (sec != shownSec) {
        shownSec = sec;
        _tft.setTextDatum(TC_DATUM);
        _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
        _tft.setTextPadding(_tft.textWidth(String(tr(S_TEST_POINT)) + "4/4: 5 s", 2));
        _tft.drawString(String(tr(S_TEST_POINT)) + (i + 1) + "/4: " + sec + " s", 160, 4, 2);
      }

      uint16_t x, y;
      if (readPoint(x, y)) {
        touched = true;
        int dx = (int)x - px, dy = (int)y - py;
        hit = dx * dx + dy * dy <= TOUCH_TEST_RADIUS * TOUCH_TEST_RADIUS;
#if TOUCH_DEBUG_LOG
        Serial.printf("[Dotyk] Test %d: punkt %d,%d dotyk %u,%u -> %s\n", i + 1, px, py, x, y,
                      hit ? "OK" : "pudlo");
#endif
        if (!hit) _tft.fillCircle(x, y, 4, TFT_RED);   // pokaż, gdzie trafił
        break;
      }
      delay(10);
    }
    _tft.setTextPadding(0);

    if (!hit) {
      if (!touched) Serial.printf("[Dotyk] Test %d: brak dotyku w czasie\n", i + 1);
      delay(800);
      return false;
    }
    drawCross(px, py, TFT_GREEN);
    delay(300);
    waitRelease();
  }
  return true;
}

// ---------------------------------------------------------------------

void Touch::title(const char* text) {
  _tft.fillScreen(TFT_BLACK);
  _tft.setTextPadding(0);
  _tft.setTextDatum(TC_DATUM);
  _tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  _tft.drawString(text, 160, 20, 4);
}

void Touch::drawCross(int x, int y, uint16_t color) {
  _tft.drawLine(x - CROSS, y, x + CROSS, y, color);
  _tft.drawLine(x, y - CROSS, x, y + CROSS, color);
  _tft.drawCircle(x, y, 3, color);
}
