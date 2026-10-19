// Pakiet UDP wysyłany przez wtyczkę ETS2/ATS (BeamDash Truck Telemetry).
// Ten sam plik ma trafić do programu ESP32 – układ bajtów musi się zgadzać.
// Little-endian (x86 i ESP32), bez wyrównania (packed).
#pragma once

#include <stdint.h>

#define TRUCK_PACKET_MAGIC   "SCSD"   // pierwsze 4 bajty – po nich ESP rozpozna grę
#define TRUCK_PACKET_VERSION 1

// game
enum : uint8_t {
  TRUCK_GAME_UNKNOWN = 0,
  TRUCK_GAME_ETS2    = 1,
  TRUCK_GAME_ATS     = 2,
};

// flags – bity stanu, świateł i kontrolek ostrzegawczych
enum : uint32_t {
  TF_PAUSED          = 1u << 0,   // gra w pauzie / menu
  TF_ENGINE          = 1u << 1,   // silnik pracuje
  TF_ELECTRIC        = 1u << 2,   // zapłon (elektryka) włączony
  TF_PARKING_BRAKE   = 1u << 3,
  TF_MOTOR_BRAKE     = 1u << 4,   // hamulec silnikowy
  TF_DIFF_LOCK       = 1u << 5,
  TF_BLINKER_L       = 1u << 6,   // lampka lewego kierunkowskazu świeci (miga)
  TF_BLINKER_R       = 1u << 7,
  TF_HAZARD          = 1u << 8,   // światła awaryjne włączone
  TF_PARKING_LIGHTS  = 1u << 9,
  TF_LOW_BEAM        = 1u << 10,
  TF_HIGH_BEAM       = 1u << 11,
  TF_BEACON          = 1u << 12,  // koguty
  TF_WARN_AIR        = 1u << 16,  // niskie ciśnienie powietrza
  TF_WARN_AIR_EMERG  = 1u << 17,  // hamowanie awaryjne z braku powietrza
  TF_WARN_FUEL       = 1u << 18,
  TF_WARN_ADBLUE     = 1u << 19,
  TF_WARN_OIL        = 1u << 20,  // ciśnienie oleju
  TF_WARN_WATER      = 1u << 21,  // temperatura płynu chłodzącego
  TF_WARN_BATTERY    = 1u << 22,
  TF_TRUCK_VALID     = 1u << 31,  // dane konfiguracji ciężarówki (nazwa, pojemności, max rpm) są aktualne
};

struct __attribute__((packed)) TruckPacket {
  char     magic[4];        // TRUCK_PACKET_MAGIC
  uint8_t  version;         // TRUCK_PACKET_VERSION
  uint8_t  game;            // TRUCK_GAME_*
  uint16_t size;            // sizeof(TruckPacket)
  uint32_t counter;         // numer pakietu
  uint32_t flags;           // TF_*

  float    speed;           // m/s, ujemna = jazda do tyłu
  float    rpm;
  float    rpmMax;          // obroty maksymalne ciężarówki (z konfiguracji), 0 = nieznane
  float    cruiseSpeed;     // m/s, 0 = tempomat wyłączony
  float    speedLimit;      // m/s, ograniczenie z nawigacji, 0 = brak

  int8_t   gear;            // bieg na desce: >0 do przodu, 0 = N, <0 = wsteczne
  uint8_t  gearsForward;    // liczba biegów do przodu
  uint8_t  gearsReverse;
  uint8_t  retarder;        // aktualny stopień retardera, 0 = wyłączony
  uint8_t  retarderSteps;   // liczba stopni retardera
  uint8_t  reserved[3];

  float    fuel;            // l
  float    fuelCapacity;    // l
  float    fuelRange;       // km
  float    adblue;          // l
  float    adblueCapacity;  // l

  float    airPressure;     // psi
  float    brakeTemp;       // °C
  float    oilTemp;         // °C
  float    oilPressure;     // psi
  float    waterTemp;       // °C
  float    batteryVoltage;  // V

  float    throttle;        // 0..1 (z tempomatem)
  float    brake;           // 0..1
  float    clutch;          // 0..1

  float    odometer;        // km
  float    navDistance;     // m do celu
  float    navTime;         // s do celu (czas w grze)

  char     brand[16];       // marka ciężarówki, np. "Scania"
  char     name[24];        // model, np. "R"
};

static_assert(sizeof(TruckPacket) == 152, "TruckPacket musi mieć 152 bajtów");
