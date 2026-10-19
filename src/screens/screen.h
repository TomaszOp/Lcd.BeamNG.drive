// Wspólny interfejs ekranów kokpitu. Dashboard trzyma po jednym obiekcie każdego ekranu,
// aktywnemu przekazuje dane; które ekrany i w jakiej kolejności – mówi GameDef gry.
#pragma once

#include <TFT_eSPI.h>

#include "../games/game_def.h"

// Dane dla ekranu – dane z gry plus wartości policzone raz w Dashboard.
struct Telemetry {
  const CarData& d;
  const GameDef& game;
  float maxRpm;     // max obroty z gry albo największe widziane w tym aucie (min. 6000)
  float rpmFrac;    // d.rpm / maxRpm
  float topSpeed;   // największa prędkość (km/h) widziana w tym aucie
  bool shift;       // czas zmienić bieg
  bool blinkOn;     // faza migania (shift light, ostrzeżenia)
};

class Screen {
public:
  explicit Screen(TFT_eSPI& tft) : _tft(tft) {}
  virtual ~Screen() = default;

  // Ekran staje się aktywny: rysuje tło i stałe napisy, tworzy sprite'y.
  virtual void enter() = 0;
  // Nowa klatka (~30/s): rysuje tylko to, co się zmieniło.
  virtual void update(const Telemetry& t) = 0;
  // Ekran przestaje być aktywny: zwalnia pamięć sprite'ów.
  virtual void leave() {}

protected:
  TFT_eSPI& _tft;
};
