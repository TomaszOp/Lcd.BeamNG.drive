// Pakiet UDP wysyłany przez mod BeamNG.drive (BeamDash protocol, mod/lua/vehicle/protocols/beamdash.lua).
// Ten sam plik ma trafić do programu ESP32 – układ bajtów musi się zgadzać z getStructDefinition() w Lua.
// Little-endian (x86 i ESP32). Gra buduje strukturę przez LuaJIT FFI ze zwykłym wyrównaniem C,
// dlatego pola są ułożone tak, żeby nie było paddingu – wersja packed i niepacked mają ten sam układ.
#pragma once

#include <stdint.h>

#define BEAM_PACKET_MAGIC   "BNGD"   // pierwsze 4 bajty – po nich ESP rozpozna grę
#define BEAM_PACKET_VERSION 1

// gearboxType – rodzaj skrzyni z powertrain.getDevice("gearbox").type
enum : uint8_t {
  BEAM_GB_UNKNOWN    = 0,
  BEAM_GB_MANUAL     = 1,
  BEAM_GB_AUTOMATIC  = 2,
  BEAM_GB_DCT        = 3,
  BEAM_GB_CVT        = 4,
  BEAM_GB_SEQUENTIAL = 5,
  BEAM_GB_ELECTRIC   = 6,   // napęd elektryczny bez skrzyni
};

// flags – bity stanu, świateł i kontrolek ostrzegawczych
enum : uint32_t {
  BF_ENGINE          = 1u << 0,   // silnik pracuje
  BF_IGNITION        = 1u << 1,   // zapłon włączony (ignitionLevel >= 2)
  BF_PARKING_BRAKE   = 1u << 2,
  BF_SIGNAL_L        = 1u << 3,   // lampka lewego kierunkowskazu świeci (miga)
  BF_SIGNAL_R        = 1u << 4,
  BF_HAZARD          = 1u << 5,   // światła awaryjne włączone
  BF_LOW_BEAM        = 1u << 6,
  BF_HIGH_BEAM       = 1u << 7,
  BF_FOG             = 1u << 8,
  BF_BEACON          = 1u << 9,   // koguty / lightbar
  BF_SHIFT           = 1u << 10,  // gra sugeruje zmianę biegu (kontroler shiftLights)
  BF_ABS             = 1u << 11,  // ABS działa w tej chwili
  BF_TC              = 1u << 12,  // kontrola trakcji działa
  BF_ESC             = 1u << 13,  // ESC działa
  BF_WARN_CHECK      = 1u << 16,  // check engine
  BF_WARN_OIL        = 1u << 17,  // brak oleju (damageTracker starvedOfOil)
  BF_WARN_WATER      = 1u << 18,  // przegrzany płyn chłodzący
  BF_WARN_OIL_TEMP   = 1u << 19,  // przegrzany olej
  BF_WARN_RADIATOR   = 1u << 20,  // wyciek z chłodnicy
  BF_WARN_FUEL       = 1u << 21,  // rezerwa paliwa
  BF_HAS_ABS         = 1u << 24,  // auto ma ABS
  BF_HAS_TC          = 1u << 25,
  BF_HAS_ESC         = 1u << 26,
  BF_HAS_TURBO       = 1u << 27,
  BF_VEHICLE_VALID   = 1u << 31,  // znaleziono silnik – rpmMax / rpmIdle / gearboxType są aktualne
};

struct __attribute__((packed)) BeamPacket {
  char     magic[4];        //  0 BEAM_PACKET_MAGIC
  uint8_t  version;         //  4 BEAM_PACKET_VERSION
  uint8_t  gearboxType;     //  5 BEAM_GB_*
  uint16_t size;            //  6 sizeof(BeamPacket)
  uint32_t counter;         //  8 numer pakietu
  uint32_t flags;           // 12 BF_*

  float    speed;           // 16 m/s
  float    rpm;             // 20
  float    rpmMax;          // 24 obroty maksymalne silnika, 0 = nieznane
  float    rpmIdle;         // 28 obroty biegu jałowego, 0 = nieznane
  float    throttle;        // 32 0..1
  float    brake;           // 36 0..1
  float    clutch;          // 40 0..1
  float    fuel;            // 44 l
  float    fuelCapacity;    // 48 l
  float    engTemp;         // 52 °C (płyn chłodzący)
  float    oilTemp;         // 56 °C
  float    turbo;           // 60 bar (nadciśnienie), 0 = brak turbo
  float    cruiseSpeed;     // 64 m/s, 0 = tempomat wyłączony
  float    envTemp;         // 68 °C, temperatura otoczenia

  int8_t   gear;            // 72 >0 do przodu, 0 = N, <0 = wsteczne
  char     gearMode;        // 73 pierwsza litera biegu z gry: 'P','R','N','D','S','M','L' albo '0' dla manuala
  uint8_t  ignition;        // 74 0 = wył., 1 = akcesoria, 2 = zapłon, 3 = rozrusznik
  uint8_t  reserved;        // 75

  char     model[24];       // 76 nazwa modelu z gry, np. "etk800", zawsze zakończona '\0'
};

static_assert(sizeof(BeamPacket) == 100, "BeamPacket musi mieć 100 bajtów");
