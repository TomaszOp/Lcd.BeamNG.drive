// Protokół OutGauge (BeamNG.drive, zgodny z Live for Speed). Parser: games/beamng_outgauge.cpp.
#pragma once

#include <stdint.h>

// Pakiet UDP – 92 bajty bez pola id, 96 z polem id. Little-endian jak ESP32.
struct __attribute__((packed)) OutGaugePacket {
  uint32_t time;          // ms
  char     car[4];        // skrót nazwy auta
  uint16_t flags;         // OG_*
  uint8_t  gear;          // 0 = R, 1 = N, 2 = 1. bieg, 3 = 2. bieg...
  uint8_t  plid;
  float    speed;         // m/s
  float    rpm;
  float    turbo;         // bar
  float    engTemp;       // °C
  float    fuel;          // 0..1
  float    oilPressure;   // bar
  float    oilTemp;       // °C
  uint32_t dashLights;    // DL_* – kontrolki dostępne w aucie
  uint32_t showLights;    // DL_* – kontrolki aktualnie zapalone
  float    throttle;      // 0..1
  float    brake;         // 0..1
  float    clutch;        // 0..1
  char     display1[16];
  char     display2[16];
  int32_t  id;
};
static_assert(sizeof(OutGaugePacket) == 96, "OutGaugePacket musi mieć 96 bajtów");

// Bity flags
enum : uint16_t {
  OG_SHIFT = 1 << 0,
  OG_CTRL  = 1 << 1,
  OG_TURBO = 1 << 13,     // auto ma turbo
  OG_KM    = 1 << 14,     // użytkownik woli km
  OG_BAR   = 1 << 15,     // użytkownik woli bar
};

// Bity dashLights / showLights
enum : uint32_t {
  DL_SHIFT      = 1 << 0,
  DL_FULLBEAM   = 1 << 1,
  DL_HANDBRAKE  = 1 << 2,
  DL_PITSPEED   = 1 << 3,
  DL_TC         = 1 << 4,
  DL_SIGNAL_L   = 1 << 5,
  DL_SIGNAL_R   = 1 << 6,
  DL_SIGNAL_ANY = 1 << 7,
  DL_OILWARN    = 1 << 8,
  DL_BATTERY    = 1 << 9,
  DL_ABS        = 1 << 10,
};
