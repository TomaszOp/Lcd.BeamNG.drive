// Euro Truck Simulator 2 / American Truck Simulator – pakiet TruckPacket z wtyczki
// ETS2Mod (ETS2Mod/src/truck_packet.h, 152 bajty, zaczyna się od "SCSD").
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "car_data.h"

bool parseEts2(const uint8_t* buf, size_t len, CarData& out);
bool parseAts(const uint8_t* buf, size_t len, CarData& out);
