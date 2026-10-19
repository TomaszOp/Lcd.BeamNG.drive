// Wspólne dane pojazdu – każdy parser gry (games/) wypełnia tę strukturę,
// a ekrany czytają tylko ją, bez wiedzy, z której gry przyszły dane.
// Wartość NAN = gra jej nie podaje (ekran pokazuje "--").
#pragma once

#include <math.h>
#include <stdint.h>

#include <initializer_list>

// Bity CarData::lights – kontrolki
enum : uint32_t {
  L_SHIFT        = 1u << 0,    // gra sygnalizuje zmianę biegu
  L_SIGNAL_L     = 1u << 1,    // kierunkowskaz (świeci w danej chwili)
  L_SIGNAL_R     = 1u << 2,
  L_HAZARD       = 1u << 3,    // światła awaryjne włączone
  L_ABS          = 1u << 4,
  L_TC           = 1u << 5,
  L_HANDBRAKE    = 1u << 6,
  L_LOW_BEAM     = 1u << 7,
  L_HIGH_BEAM    = 1u << 8,
  L_PARKING      = 1u << 9,    // światła pozycyjne
  L_BEACON       = 1u << 10,   // koguty
  L_ENGINE_BRAKE = 1u << 11,
  L_OIL_WARN     = 1u << 12,
  L_BATTERY      = 1u << 13,
  L_WATER_WARN   = 1u << 14,
  L_FUEL_WARN    = 1u << 15,
  L_AIR_WARN     = 1u << 16,   // niskie ciśnienie powietrza w hamulcach
  L_AIR_EMERG    = 1u << 17,   // hamowanie awaryjne z braku powietrza
  L_ADBLUE_WARN  = 1u << 18,
};

struct CarData {
  // wszystkie gry
  float speed = 0;          // km/h
  float rpm = 0;
  float rpmMax = 0;         // max obroty z gry, 0 = gra nie podaje (Dashboard uczy się zakresu)
  int8_t gear = 0;          // >0 do przodu, 0 = N, <0 wsteczne (-1 = R, -2 = R2...)
  float throttle = 0, brake = 0, clutch = 0;   // 0..1
  float fuel = NAN;         // 0..1
  float engTemp = NAN;      // °C (płyn chłodzący)
  float oilTemp = NAN;      // °C
  float oilPressure = NAN;  // bar
  float turbo = NAN;        // bar
  uint32_t lights = 0;      // L_*
  uint32_t carKey = 0;      // "odcisk" pojazdu – zmiana = inne auto (zakres obrotów od nowa)

  // ciężarówki (NAN = brak)
  float cruiseSpeed = NAN;  // km/h, 0 = tempomat wyłączony
  float speedLimit = NAN;   // km/h, 0 = brak ograniczenia
  float fuelRange = NAN;    // km
  float adblue = NAN;       // 0..1
  float airPressure = NAN;  // bar
  float navDistance = NAN;  // km do celu
  float navTime = NAN;      // minuty do celu
  uint8_t retarder = 0, retarderSteps = 0;   // retarderSteps 0 = brak retardera
};

// Zabezpieczenie przed uszkodzonym pakietem: NaN / nieskończoność zamienione na wartości,
// które ekrany umieją narysować. Pola zawsze obecne -> 0, opcjonalne -> NAN ("--").
// Wywołuje GameReceiver po każdym parserze – nowa gra nie musi o tym pamiętać.
inline void sanitize(CarData& d) {
  for (float* v : {&d.speed, &d.rpm, &d.rpmMax, &d.throttle, &d.brake, &d.clutch})
    if (!isfinite(*v)) *v = 0;
  for (float* v : {&d.fuel, &d.engTemp, &d.oilTemp, &d.oilPressure, &d.turbo, &d.cruiseSpeed,
                   &d.speedLimit, &d.fuelRange, &d.adblue, &d.airPressure, &d.navDistance, &d.navTime})
    if (!isfinite(*v)) *v = NAN;
}
