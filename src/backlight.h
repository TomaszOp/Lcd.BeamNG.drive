// Podświetlenie wyświetlacza (pin TFT_BL) sterowane PWM, poziom w % zapisany w NVS.
#pragma once

#include <Arduino.h>

class Backlight {
public:
  static const uint8_t MIN_LEVEL = 10;   // niżej ekran jest praktycznie nieczytelny
  static const uint8_t STEP = 10;

  // Wywołać po Dashboard::begin() (TFT_eSPI ustawia pin podświetlenia w init()).
  void begin();

  uint8_t level() const { return _level; }
  void set(uint8_t percent);   // ustawia od razu, bez zapisu
  void save();                 // zapisuje aktualny poziom w NVS
  static void clear();         // kasuje zapisany poziom (reset urządzenia)

private:
  uint8_t _level = 100;
};
