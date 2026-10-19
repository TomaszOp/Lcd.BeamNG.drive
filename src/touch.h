// Ekran dotykowy XPT2046 (przez TFT_eSPI, wspólna szyna SPI z wyświetlaczem).
//  - na starcie wczytuje kalibrację z pamięci NVS,
//  - brak kalibracji -> kalibracja (4 rogi) + test (4 losowe punkty, po 5 s na każdy),
//  - test udany -> kalibracja zapisana, dotyk włączony,
//  - test nieudany -> nic nie zapisujemy, dotyk wyłączony (enabled() == false),
//    program startuje dalej; przy następnym uruchomieniu kalibracja ruszy znowu.
#pragma once

#include <TFT_eSPI.h>

class Touch {
public:
  explicit Touch(TFT_eSPI& tft) : _tft(tft) {}

  // Wywołać po Dashboard::begin() (wyświetlacz zainicjowany). Może blokować na czas kalibracji.
  void begin();

  // Czy dotyk działa (jest poprawna kalibracja).
  bool enabled() const { return _enabled; }

  // Odczyt dotyku we współrzędnych ekranu; false gdy brak dotyku albo dotyk wyłączony.
  bool read(uint16_t& x, uint16_t& y);

  // Kalibracja na życzenie (z ekranu konfiguracji): kalibracja + test.
  // Udana -> zapis i dotyk włączony; nieudana -> zostaje poprzednia kalibracja.
  bool recalibrate();

  // Kasuje zapisaną kalibrację (reset urządzenia) – po restarcie kalibracja ruszy od nowa.
  static void clearCalibration();

private:
  static const uint8_t CAL_SIZE = 5;   // format danych TFT_eSPI::setTouch()

  bool loadCalibration(uint16_t* data);
  void saveCalibration(const uint16_t* data);

  bool calibrate(uint16_t* data);      // 4 rogi -> dane kalibracji (ustawione też w TFT_eSPI)
  bool runTest();                      // 4 losowe punkty do trafienia

  bool readCorner(int px, int py, uint16_t& rawX, uint16_t& rawY);
  bool readRaw(uint16_t& rawX, uint16_t& rawY, uint16_t threshold);   // uśrednione surowe x, y
  bool readPoint(uint16_t& x, uint16_t& y);   // szybki odczyt we współrzędnych ekranu
  bool pressed(uint16_t threshold);    // surowy nacisk z powyżej progu
  void waitRelease();

  bool calibrateAndTest(uint16_t* data);   // kalibracja + test + ekran z wynikiem
  void title(const char* text);
  void drawCross(int x, int y, uint16_t color);

  TFT_eSPI& _tft;
  bool _enabled = false;
  uint16_t _cal[CAL_SIZE] = {};        // aktualna kalibracja (gdy _enabled)
};
