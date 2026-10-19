// Ekran 4 – pedały: pionowe paski gazu, hamulca i sprzęgła
// oraz przewijany wykres gazu i hamulca z ostatnich kilku sekund.
#pragma once

#include "screen.h"

class PedalsScreen : public Screen {
public:
  using Screen::Screen;

  void enter() override;
  void update(const Telemetry& t) override;
  void leave() override;

private:
  void drawGraphColumn(float throttle, float brake);

  TFT_eSprite _graph{&_tft};
  int _prevThrottleY = -1, _prevBrakeY = -1;

  int32_t _shownBar[3];
  uint32_t _shownLights;
};
