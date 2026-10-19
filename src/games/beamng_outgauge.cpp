#include "beamng_outgauge.h"

#include <string.h>

#include "../outgauge.h"
#include "beamng_mod.h"

// bity OutGauge -> wspólne kontrolki
static const struct { uint32_t og, car; } LIGHT_MAP[] = {
    {DL_SHIFT, L_SHIFT},         {DL_SIGNAL_L, L_SIGNAL_L}, {DL_SIGNAL_R, L_SIGNAL_R},
    {DL_ABS, L_ABS},             {DL_TC, L_TC},             {DL_HANDBRAKE, L_HANDBRAKE},
    {DL_FULLBEAM, L_HIGH_BEAM},  {DL_OILWARN, L_OIL_WARN},  {DL_BATTERY, L_BATTERY},
};

bool parseBeamNG(const uint8_t* buf, size_t len, CarData& out) {
  if (len != 92 && len != 96) return false;
  if (beamngModActive()) return false;   // mod wysyła pełniejsze dane – OutGauge pomijamy
  OutGaugePacket p{};
  memcpy(&p, buf, len);

  out = CarData();
  out.speed = p.speed * 3.6f;
  out.rpm = p.rpm;
  out.rpmMax = 0;   // OutGauge nie podaje – zakres uczony (OPT_AUTO_RPM)
  out.gear = p.gear == 0 ? -1 : p.gear - 1;   // OutGauge: 0 = R, 1 = N, 2 = 1. bieg
  out.throttle = p.throttle;
  out.brake = p.brake;
  out.clutch = p.clutch;
  out.fuel = p.fuel;
  out.engTemp = p.engTemp;
  out.oilTemp = p.oilTemp;
  out.oilPressure = p.oilPressure;   // BeamNG zawsze wysyła 0
  out.turbo = p.turbo;
  for (const auto& m : LIGHT_MAP)
    if (p.showLights & m.og) out.lights |= m.car;

  // BeamNG wpisuje w pole car zawsze "beam", więc auto rozpoznajemy po
  // kontrolkach dostępnych w aucie + czy ma turbo
  out.carKey = p.dashLights | (p.flags & OG_TURBO);
  return true;
}
