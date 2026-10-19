// Lista obsługiwanych gier. Kolejność ma znaczenie: odbiornik pyta parsery po kolei,
// pierwszy, który rozpozna pakiet, wygrywa.
//
// Nowa gra:
//   1. parser w games/<gra>.cpp (sprawdza rozmiar/nagłówek pakietu, wypełnia CarData),
//   2. wpis poniżej: nazwa, podpowiedź, parser, ekrany w kolejności przełączania, opcje.
#include "game_def.h"

#include "beamng_mod.h"
#include "beamng_outgauge.h"
#include "scs_telemetry.h"

using S = ScreenId;

const GameDef GAMES[] = {
  // mod BeamNGMod podaje max obroty – bez OPT_AUTO_RPM
  {"BeamNG.drive (mod)", S_HINT_BEAMNG, parseBeamNGMod,
   {S::Race, S::Gauges, S::Engine, S::Pedals}, 0},

  {"BeamNG.drive", S_HINT_BEAMNG, parseBeamNG,
   {S::Race, S::Gauges, S::Engine, S::Pedals}, OPT_AUTO_RPM},

  {"ETS2", S_HINT_SCS, parseEts2,
   {S::Race, S::Gauges, S::Engine, S::Pedals, S::Truck}, OPT_SPEED_140},

  {"ATS", S_HINT_SCS, parseAts,
   {S::Race, S::Gauges, S::Engine, S::Pedals, S::Truck}, OPT_SPEED_140},
};

const int GAME_COUNT = sizeof(GAMES) / sizeof(GAMES[0]);
