// Ekran 2 – zegary analogowe: prędkościomierz i obrotomierz ze wskazówkami.
// Wskazówki "dojeżdżają" do wartości przez kilka klatek, jak mechaniczne.
// Skala prędkości rośnie stopniami (160..400 km/h) wraz z największą prędkością auta;
// dla ciężarówek (OPT_SPEED_140) stała 0–140 km/h.
#pragma once

#include "screen.h"

class GaugesScreen : public Screen {
public:
  using Screen::Screen;

  void enter() override;
  void update(const Telemetry& t) override;
  void leave() override;

private:
  struct DialScale {
    float maxValue;
    float minorStep;   // co ile krótka kreska
    float majorStep;   // co ile długa kreska
    float labelStep;   // co ile liczba
    float labelDiv;    // liczba = wartość / labelDiv
    float redFrom;     // od jakiej części skali czerwone pole (1 = brak)
  };

  void drawFace(TFT_eSprite& s, const DialScale& scale);
  void drawNeedle(TFT_eSprite& s, float frac);
  void drawSpeedDial(const Telemetry& t);
  static const DialScale& speedScale(const Telemetry& t);
  void drawRpmDial(const Telemetry& t);

  TFT_eSprite _speedDial{&_tft};
  TFT_eSprite _rpmDial{&_tft};

  float _speedNeedle = 0;   // pozycja wskazówki (km/h) – animowana
  float _rpmNeedle = 0;     // pozycja wskazówki (rpm) – animowana

  int32_t _shownSpeedNeedle, _shownSpeed;
  float _shownSpeedMax;
  int32_t _shownRpmNeedle, _shownRpmKey;
  int32_t _shownThrottle, _shownBrake;
  uint32_t _shownLights;
};
