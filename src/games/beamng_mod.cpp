#include "beamng_mod.h"

#include <Arduino.h>
#include <string.h>

// ten sam plik co w modzie – układ pakietu w jednym miejscu
#include "../../BeamNGMod/src/beamng_packet.h"

static const float MS_TO_KMH = 3.6f;
static const uint32_t MOD_ACTIVE_MS = 1000;   // tyle po ostatnim pakiecie modu ignorujemy OutGauge

static uint32_t lastModPacketMs = 0;
static bool modSeen = false;

// bity modu -> wspólne kontrolki
static const struct { uint32_t bf, car; } LIGHT_MAP[] = {
    {BF_SIGNAL_L, L_SIGNAL_L},       {BF_SIGNAL_R, L_SIGNAL_R},   {BF_HAZARD, L_HAZARD},
    {BF_PARKING_BRAKE, L_HANDBRAKE}, {BF_LOW_BEAM, L_LOW_BEAM},   {BF_HIGH_BEAM, L_HIGH_BEAM},
    {BF_BEACON, L_BEACON},           {BF_SHIFT, L_SHIFT},         {BF_ABS, L_ABS},
    {BF_TC, L_TC},                   {BF_WARN_OIL, L_OIL_WARN},   {BF_WARN_WATER, L_WATER_WARN},
    {BF_WARN_FUEL, L_FUEL_WARN},
};

static float ratio(float value, float capacity) {
  return capacity > 0 ? value / capacity : NAN;
}

// odcisk auta: model + max obroty + rodzaj skrzyni
static uint32_t carKey(const BeamPacket& p) {
  uint32_t h = 2166136261u;   // FNV-1a
  for (size_t i = 0; i < sizeof(p.model) && p.model[i]; i++) h = (h ^ (uint8_t)p.model[i]) * 16777619u;
  return h ^ (uint32_t)p.rpmMax ^ ((uint32_t)p.gearboxType << 24);
}

bool parseBeamNGMod(const uint8_t* buf, size_t len, CarData& out) {
  if (len != sizeof(BeamPacket) || memcmp(buf, BEAM_PACKET_MAGIC, 4) != 0) return false;
  BeamPacket p;
  memcpy(&p, buf, sizeof(p));
  if (p.version != BEAM_PACKET_VERSION || p.size != sizeof(BeamPacket)) return false;

  lastModPacketMs = millis();
  modSeen = true;

  out = CarData();
  out.speed = fabsf(p.speed) * MS_TO_KMH;
  out.rpm = p.rpm;
  out.rpmMax = (p.flags & BF_VEHICLE_VALID) ? p.rpmMax : 0;   // 0 = Dashboard uczy się zakresu
  out.gear = p.gear;
  out.throttle = p.throttle;
  out.brake = p.brake;
  out.clutch = p.clutch;
  out.fuel = ratio(p.fuel, p.fuelCapacity);
  out.engTemp = p.engTemp;
  out.oilTemp = p.oilTemp;
  // oilPressure zostaje NAN – BeamNG go nie podaje
  out.turbo = (p.flags & BF_HAS_TURBO) ? p.turbo : NAN;
  for (const auto& m : LIGHT_MAP)
    if (p.flags & m.bf) out.lights |= m.car;
  out.carKey = carKey(p);
  out.cruiseSpeed = p.cruiseSpeed * MS_TO_KMH;
  return true;
}

bool beamngModActive() {
  return modSeen && millis() - lastModPacketMs < MOD_ACTIVE_MS;
}
