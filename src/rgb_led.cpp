#include "rgb_led.h"

void RgbLed::begin() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  off();
}

void RgbLed::set(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t color = (uint32_t)r << 16 | (uint32_t)g << 8 | b;
  if (color == _current) return;
  _current = color;

#if LED_COMMON_ANODE
  r = 255 - r;
  g = 255 - g;
  b = 255 - b;
#endif
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
  analogWrite(PIN_LED_B, b);
}
