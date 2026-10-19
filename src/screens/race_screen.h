// Ekran 1 – wyścigowy: pasek obrotów, duży bieg, prędkość i obroty cyfrowo,
// kontrolki, paliwo/temperatury/turbo, paski pedałów.
#pragma once

#include "screen.h"

class RaceScreen : public Screen {
public:
  using Screen::Screen;

  void enter() override;
  void update(const Telemetry& t) override;
  void leave() override;

private:
  // Ostatnio narysowane wartości – rysujemy tylko to, co się zmieniło.
  struct Shown {
    int32_t rpmBar, gear, speed, rpm, fuel, engTemp, oilTemp, turbo10;
    int32_t throttle, brake, clutch;
    uint32_t lights;
  };

  void drawRpmBar(const Telemetry& t);
  void drawGear(const Telemetry& t);
  void drawInfoValue(int line, int32_t key, int32_t& shown, const char* text);
  void drawPedal(int row, float value, int32_t& shown, uint16_t color);

  TFT_eSprite _rpmSprite{&_tft};
  TFT_eSprite _gearSprite{&_tft};
  Shown _shown;
};
