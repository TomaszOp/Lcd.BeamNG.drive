// Ekran 3 – silnik i auto: paliwo, temperatury, ciśnienie oleju, turbo
// jako paski z wartościami; kolor ostrzega o przekroczonych progach.
#pragma once

#include "screen.h"

class EngineScreen : public Screen {
public:
  using Screen::Screen;

  void enter() override;
  void update(const Telemetry& t) override;

private:
  static const int ROW_COUNT = 5;

  int32_t _shownKey[ROW_COUNT];   // wartość ×10 + stan migania
  uint32_t _shownLamps;
};
