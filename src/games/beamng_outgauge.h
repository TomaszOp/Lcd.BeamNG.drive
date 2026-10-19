// BeamNG.drive – protokół OutGauge (pakiet 92 lub 96 bajtów, patrz outgauge.h).
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "car_data.h"

bool parseBeamNG(const uint8_t* buf, size_t len, CarData& out);
