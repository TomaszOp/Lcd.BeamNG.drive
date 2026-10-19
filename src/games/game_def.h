// Opis obsługiwanej gry: jak rozpoznać jej pakiet, które ekrany pokazywać i w jakiej
// kolejności, oraz opcje. Lista gier jest w games/game_list.cpp – nowa gra to nowy
// parser (games/*.cpp) i jeden wpis na liście.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "car_data.h"
#include "../lang.h"

// Ekrany kokpitu (Dashboard ma po jednym obiekcie każdego)
enum class ScreenId : uint8_t {
  None = 0,   // koniec listy
  Race,       // wyścigowy: pasek obrotów, bieg, prędkość, pedały
  Gauges,     // zegary analogowe
  Engine,     // paliwo, temperatury, ciśnienie, turbo
  Pedals,     // pedały z wykresem
  Truck,      // ciężarówka: tempomat, limit, powietrze, AdBlue, retarder, nawigacja
};

// Opcje gry (bity GameDef::options)
enum : uint8_t {
  OPT_AUTO_RPM  = 1 << 0,   // gra nie podaje max obrotów – zakres uczony z jazdy (min. 6000)
  OPT_SPEED_140 = 1 << 1,   // prędkościomierz ze stałą skalą 0–140 km/h (ciężarówki z ogranicznikiem)
};

// Parser: true, jeśli pakiet należy do tej gry i dane są aktualne (nie pauza) –
// wtedy wypełnia out. Pakiet innej gry albo pauza -> false.
typedef bool (*GameParser)(const uint8_t* buf, size_t len, CarData& out);

static const int MAX_GAME_SCREENS = 8;

struct GameDef {
  const char* name;                     // pokazywana na ekranie i w logu
  StrId setupHint;                      // podpowiedź (tekst z lang.h) na ekranie "Czekam na dane z gry"
  GameParser parse;
  ScreenId screens[MAX_GAME_SCREENS];   // kolejność przełączania; reszta = None
  uint8_t options;                      // OPT_*

  int screenCount() const {
    int n = 0;
    while (n < MAX_GAME_SCREENS && screens[n] != ScreenId::None) n++;
    return n;
  }
};

extern const GameDef GAMES[];
extern const int GAME_COUNT;
