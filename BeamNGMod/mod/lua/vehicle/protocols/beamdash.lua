-- BeamDash – protokół telemetrii BeamNG.drive dla wyświetlacza Lcd.BeamNG.drive (ESP32).
-- Rozszerza OutGauge o dane, których ten nie wysyła: max/idle RPM, rodzaj skrzyni i tryb (P/R/N/D/S/M),
-- światła drogowe/mijania/przeciwmgielne, check engine, ostrzeżenia olej/temperatura, tempomat, model auta.
--
-- Układ pakietu musi być taki sam jak w ../../../../src/beamng_packet.h (100 B, magic "BNGD").
-- Interfejs modułu (init, reset, getAddress, getPort, getMaxUpdateRate, isPhysicsStepUsed,
-- getStructDefinition, fillStruct) – jak we wbudowanym lua/vehicle/protocols/outgauge.lua.
-- Dokumentacja: https://go.beamng.com/protocols

local M = {}

-- ===== Konfiguracja =========================================================================
-- IP wyświetlacza. nil = adres OutGauge z menu gry (Options > Other > Protocols).
local ESP_IP   = nil          -- np. "192.168.1.50"
local ESP_PORT = 4444         -- ten sam port co OutGauge i wtyczka ETS2 – ESP rozpozna pakiet po magic
local RATE_HZ  = 30           -- pakiety na sekundę
-- ============================================================================================

local PACKET_VERSION = 1
local PACKET_SIZE    = 100

-- BF_* – bity flags, jak w beamng_packet.h
local BF_ENGINE        = 2 ^ 0
local BF_IGNITION      = 2 ^ 1
local BF_PARKING_BRAKE = 2 ^ 2
local BF_SIGNAL_L      = 2 ^ 3
local BF_SIGNAL_R      = 2 ^ 4
local BF_HAZARD        = 2 ^ 5
local BF_LOW_BEAM      = 2 ^ 6
local BF_HIGH_BEAM     = 2 ^ 7
local BF_FOG           = 2 ^ 8
local BF_BEACON        = 2 ^ 9
local BF_SHIFT         = 2 ^ 10
local BF_ABS           = 2 ^ 11
local BF_TC            = 2 ^ 12
local BF_ESC           = 2 ^ 13
local BF_WARN_CHECK    = 2 ^ 16
local BF_WARN_OIL      = 2 ^ 17
local BF_WARN_WATER    = 2 ^ 18
local BF_WARN_OIL_TEMP = 2 ^ 19
local BF_WARN_RADIATOR = 2 ^ 20
local BF_WARN_FUEL     = 2 ^ 21
local BF_HAS_ABS       = 2 ^ 24
local BF_HAS_TC        = 2 ^ 25
local BF_HAS_ESC       = 2 ^ 26
local BF_HAS_TURBO     = 2 ^ 27
local BF_VEHICLE_VALID = 2 ^ 31

-- BEAM_GB_* – typ urządzenia "gearbox" w powertrain -> numer w pakiecie
local GEARBOX_TYPES = {
  manualGearbox     = 1,
  automaticGearbox  = 2,
  dctGearbox        = 3,
  cvtGearbox        = 4,
  sequentialGearbox = 5,
}
local GB_ELECTRIC = 6

-- dane auta odczytywane raz, w init()
local vehicleValid, gearboxType, rpmMax, rpmIdle, model = false, 0, 0, 0, ""
local counter = 0

-- electrics.values bywają liczbą (0/1) albo bool – wspólne sprawdzenie "włączone"
local function on(x)
  return x == true or (type(x) == "number" and x ~= 0)
end

local function damage(group, name)
  local d = damageTracker and damageTracker.getDamage(group, name)
  return d == true or (type(d) == "number" and d > 0)
end

local function init()
  vehicleValid, gearboxType, rpmMax, rpmIdle, model = false, 0, 0, 0, ""

  local engine = powertrain and powertrain.getDevice("mainEngine")
  if engine then
    vehicleValid = true
    rpmMax  = tonumber(engine.maxRPM) or 0
    rpmIdle = tonumber(engine.idleRPM) or 0
  end

  local gearbox = powertrain and powertrain.getDevice("gearbox")
  if gearbox then
    gearboxType = GEARBOX_TYPES[gearbox.type] or 0
  elseif engine and engine.type == "electricMotor" then
    gearboxType = GB_ELECTRIC
  end

  if v and v.config and type(v.config.mainPartName) == "string" then
    model = v.config.mainPartName
  end

  log("I", "beamdash", string.format("model=%s rpmMax=%d rpmIdle=%d gearbox=%d -> %s:%d",
      model, rpmMax, rpmIdle, gearboxType, tostring(M.getAddress()), ESP_PORT))
end

local function reset() end

local function getAddress()
  return ESP_IP or settings.getValue("protocols_outgauge_address")
end

local function getPort()           return ESP_PORT end
local function getMaxUpdateRate()  return RATE_HZ end
local function isPhysicsStepUsed() return false end   -- krok grafiki, tanio; i tak max RATE_HZ

-- 100 B, pola ułożone bez paddingu (patrz beamng_packet.h)
local function getStructDefinition()
  return [[
    char     magic[4];
    uint8_t  version;
    uint8_t  gearboxType;
    uint16_t size;
    uint32_t counter;
    uint32_t flags;
    float    speed;
    float    rpm;
    float    rpmMax;
    float    rpmIdle;
    float    throttle;
    float    brake;
    float    clutch;
    float    fuel;
    float    fuelCapacity;
    float    engTemp;
    float    oilTemp;
    float    turbo;
    float    cruiseSpeed;
    float    envTemp;
    int8_t   gear;
    char     gearMode;
    uint8_t  ignition;
    uint8_t  reserved;
    char     model[24];
  ]]
end

local function fillStruct(o, dtSim)
  local e = electrics.values
  if not e.watertemp then return end   -- pojazd jeszcze się nie zainicjował – nie wysyłaj

  counter = (counter + 1) % 4294967296

  local f = 0
  local function set(cond, bitValue) if cond then f = f + bitValue end end

  set(on(e.engineRunning), BF_ENGINE)
  set((e.ignitionLevel or 0) >= 2, BF_IGNITION)
  set(on(e.parkingbrake), BF_PARKING_BRAKE)
  set(on(e.signal_L), BF_SIGNAL_L)
  set(on(e.signal_R), BF_SIGNAL_R)
  set(on(e.hazard_enabled), BF_HAZARD)
  set(on(e.lowbeam), BF_LOW_BEAM)
  set(on(e.highbeam), BF_HIGH_BEAM)
  set(on(e.fog), BF_FOG)
  set(on(e.lightbar), BF_BEACON)
  set(on(e.shouldShift), BF_SHIFT)
  set(on(e.hasABS) and on(e.absActive), BF_ABS)
  set(on(e.hasTCS) and on(e.tcsActive), BF_TC)
  set(on(e.hasESC) and on(e.escActive), BF_ESC)
  set(on(e.checkengine), BF_WARN_CHECK)
  set(damage("engine", "starvedOfOil"), BF_WARN_OIL)
  set(damage("engine", "coolantOverheating"), BF_WARN_WATER)
  set(damage("engine", "oilOverheating"), BF_WARN_OIL_TEMP)
  set(damage("engine", "radiatorLeak"), BF_WARN_RADIATOR)
  set(on(e.lowfuel), BF_WARN_FUEL)
  set(on(e.hasABS), BF_HAS_ABS)
  set(on(e.hasTCS), BF_HAS_TC)
  set(on(e.hasESC), BF_HAS_ESC)
  set(e.turboBoost ~= nil, BF_HAS_TURBO)
  set(vehicleValid, BF_VEHICLE_VALID)

  local gearStr = tostring(e.gear or "")
  local gearChar = string.sub(gearStr, 1, 1)
  if gearChar == "" or tonumber(gearChar) ~= nil or gearChar == "-" then gearChar = "0" end

  o.magic        = "BNGD"
  o.version      = PACKET_VERSION
  o.gearboxType  = gearboxType
  o.size         = PACKET_SIZE
  o.counter      = counter
  o.flags        = f
  o.speed        = e.wheelspeed or e.airspeed or 0
  o.rpm          = e.rpm or 0
  o.rpmMax       = rpmMax
  o.rpmIdle      = rpmIdle
  o.throttle     = e.throttle or 0
  o.brake        = e.brake or 0
  o.clutch       = e.clutch or 0
  o.fuel         = e.fuelVolume or 0
  o.fuelCapacity = e.fuelCapacity or 0
  o.engTemp      = e.watertemp or 0
  o.oilTemp      = e.oiltemp or 0
  o.turbo        = (e.turboBoost or 0) / 14.504   -- psi -> bar, jak w outgauge.lua
  o.cruiseSpeed  = on(e.cruiseControlActive) and (e.cruiseControlTarget or 0) or 0
  o.envTemp      = (obj and obj.getEnvTemperature) and (obj:getEnvTemperature() - 273.15) or 20
  o.gear         = math.max(-127, math.min(127, e.gearIndex or 0))
  o.gearMode     = string.byte(gearChar)
  o.ignition     = math.max(0, math.min(3, e.ignitionLevel or 0))
  o.reserved     = 0
  o.model        = string.sub(model, 1, 23)   -- zawsze zostaje miejsce na '\0'
end

M.init = init
M.reset = reset
M.getAddress = getAddress
M.getPort = getPort
M.getMaxUpdateRate = getMaxUpdateRate
M.isPhysicsStepUsed = isPhysicsStepUsed
M.getStructDefinition = getStructDefinition
M.fillStruct = fillStruct

return M
