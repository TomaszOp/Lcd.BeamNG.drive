#include "scs_telemetry.h"

#include <string.h>

// ten sam plik co we wtyczce – układ pakietu w jednym miejscu
#include "../../ETS2Mod/src/truck_packet.h"

static const float PSI_TO_BAR = 0.0689476f;
static const float MS_TO_KMH = 3.6f;

// bity wtyczki -> wspólne kontrolki
static const struct { uint32_t tf, car; } LIGHT_MAP[] = {
    {TF_BLINKER_L, L_SIGNAL_L},     {TF_BLINKER_R, L_SIGNAL_R},    {TF_HAZARD, L_HAZARD},
    {TF_PARKING_BRAKE, L_HANDBRAKE}, {TF_MOTOR_BRAKE, L_ENGINE_BRAKE}, {TF_LOW_BEAM, L_LOW_BEAM},
    {TF_HIGH_BEAM, L_HIGH_BEAM},    {TF_PARKING_LIGHTS, L_PARKING}, {TF_BEACON, L_BEACON},
    {TF_WARN_OIL, L_OIL_WARN},      {TF_WARN_BATTERY, L_BATTERY},  {TF_WARN_WATER, L_WATER_WARN},
    {TF_WARN_FUEL, L_FUEL_WARN},    {TF_WARN_AIR, L_AIR_WARN},     {TF_WARN_AIR_EMERG, L_AIR_EMERG},
    {TF_WARN_ADBLUE, L_ADBLUE_WARN},
};

static float ratio(float value, float capacity) {
  return capacity > 0 ? value / capacity : NAN;
}

// prosty odcisk ciężarówki: marka + model + max obroty
static uint32_t truckKey(const TruckPacket& p) {
  uint32_t h = 2166136261u;   // FNV-1a
  auto mix = [&h](const char* s, size_t n) {
    for (size_t i = 0; i < n && s[i]; i++) h = (h ^ (uint8_t)s[i]) * 16777619u;
  };
  mix(p.brand, sizeof(p.brand));
  mix(p.name, sizeof(p.name));
  return h ^ (uint32_t)p.rpmMax;
}

static bool parseScs(const uint8_t* buf, size_t len, CarData& out, uint8_t game) {
  if (len != sizeof(TruckPacket) || memcmp(buf, TRUCK_PACKET_MAGIC, 4) != 0) return false;
  TruckPacket p;
  memcpy(&p, buf, sizeof(p));
  if (p.version != TRUCK_PACKET_VERSION || p.game != game) return false;
  if (p.flags & TF_PAUSED) return false;   // pauza = brak danych (po chwili ekran "Czekam na dane")

  out = CarData();
  out.speed = fabsf(p.speed) * MS_TO_KMH;
  out.rpm = p.rpm;
  out.rpmMax = p.rpmMax;
  out.gear = p.gear;
  out.throttle = p.throttle;
  out.brake = p.brake;
  out.clutch = p.clutch;
  out.fuel = ratio(p.fuel, p.fuelCapacity);
  out.engTemp = p.waterTemp;
  out.oilTemp = p.oilTemp;
  out.oilPressure = p.oilPressure * PSI_TO_BAR;
  for (const auto& m : LIGHT_MAP)
    if (p.flags & m.tf) out.lights |= m.car;
  out.carKey = truckKey(p);

  out.cruiseSpeed = p.cruiseSpeed * MS_TO_KMH;
  out.speedLimit = p.speedLimit * MS_TO_KMH;
  out.fuelRange = p.fuelRange;
  out.adblue = ratio(p.adblue, p.adblueCapacity);
  out.airPressure = p.airPressure * PSI_TO_BAR;
  out.navDistance = p.navDistance / 1000.0f;
  out.navTime = p.navTime / 60.0f;
  out.retarder = p.retarder;
  out.retarderSteps = p.retarderSteps;
  return true;
}

bool parseEts2(const uint8_t* buf, size_t len, CarData& out) { return parseScs(buf, len, out, TRUCK_GAME_ETS2); }
bool parseAts(const uint8_t* buf, size_t len, CarData& out) { return parseScs(buf, len, out, TRUCK_GAME_ATS); }
