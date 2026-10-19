// Dioda RGB sterowana PWM (piny i typ wspólnej elektrody w config.h).
#pragma once

#include <Arduino.h>

class RgbLed {
public:
  void begin();
  void set(uint8_t r, uint8_t g, uint8_t b);
  void off() { set(0, 0, 0); }

private:
  uint32_t _current = 0xFFFFFFFF;   // ostatni ustawiony kolor (0xRRGGBB)
};
