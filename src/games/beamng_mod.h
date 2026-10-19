// BeamNG.drive – pakiet BeamPacket z modu BeamNGMod (BeamNGMod/src/beamng_packet.h,
// 100 bajtów, zaczyna się od "BNGD"). Więcej danych niż OutGauge, m.in. max obroty.
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "car_data.h"

bool parseBeamNGMod(const uint8_t* buf, size_t len, CarData& out);

// true, jeśli niedawno przyszedł pakiet z modu – wtedy parser OutGauge pomija swoje pakiety,
// żeby przy włączonych obu protokołach dane (i zakres obrotów) nie skakały między nimi
bool beamngModActive();
