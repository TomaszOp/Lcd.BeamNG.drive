// Ekran ciężarówki (ETS2 / ATS): tempomat, limit prędkości, ciśnienie powietrza,
// AdBlue, retarder, zasięg na paliwie, odległość i czas do celu oraz kontrolki.
#pragma once

#include "screen.h"

class TruckScreen : public Screen {
public:
  using Screen::Screen;

  void enter() override;
  void update(const Telemetry& t) override;

private:
  static const int CELL_COUNT = 8;

  void drawCell(int index, const String& text, uint16_t color);

  String _shownText[CELL_COUNT];
  uint16_t _shownColor[CELL_COUNT];
  uint32_t _shownLamps;
};
