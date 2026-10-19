# BeamNGMod – telemetry protocol mod for BeamNG.drive

A BeamNG.drive mod that adds a custom UDP telemetry protocol (`BeamPacket`, 100 bytes, see
[`src/beamng_packet.h`](src/beamng_packet.h)) for the Lcd.BeamNG.drive display. It works like the
built-in OutGauge protocol, but also sends what OutGauge lacks: maximum and idle RPM, gearbox type
and mode (P/R/N/D/S/M), low/high/fog lights, check engine, oil and temperature warnings, cruise
control and the vehicle model name.

The ESP32 firmware recognises the packet in [`src/games/beamng_mod.cpp`](../src/games/beamng_mod.cpp)
(game "BeamNG.drive (mod)"). OutGauge can stay enabled: while mod packets arrive the display ignores
OutGauge packets.

It uses the official custom protocol mechanism available since **BeamNG.drive v0.32**: every `.lua`
file in `lua/vehicle/protocols/` (also inside a mod) is loaded as a protocol for the player's vehicle
([BeamNG documentation – Protocols](https://documentation.beamng.com/modding/protocols/)). The game
builds the packet from the struct returned by `getStructDefinition()` (LuaJIT FFI), calls
`fillStruct()` and sends the raw bytes over UDP. No game files are overwritten.

## Installing (Windows)

The BeamNG user folder is usually `%LOCALAPPDATA%\BeamNG\BeamNG.drive\current\`
(on Steam).

Either, for development:

1. Copy the [`mod`](mod) folder to `<user folder>\mods\unpacked\` and rename it to `beamdash`, so the
   file ends up at `mods\unpacked\beamdash\lua\vehicle\protocols\beamdash.lua`.

or, for normal use:

1. Run `make` (on Linux) and copy `build/beamdash.zip` to `<user folder>\mods\` as it is, without
   extracting. You can also create the zip in game: Mods Manager > beamdash > PACK.

Then:

2. Make sure the mod is active in the Mods Manager.
3. Set the display IP: either `ESP_IP` at the top of `beamdash.lua`, or leave it `nil` and the mod
   uses the OutGauge address from Options > Other > Protocols.
4. Spawn a vehicle. After every change of the `.lua` file press **Ctrl+R** (reload vehicle).

The mod writes one line to the game log (console `~` and `beamng.log` in the user folder) when a
vehicle loads, starting with `beamdash`: model, max/idle RPM, gearbox type and the target address.
Lua errors are shown in the same console.

## Configuration – top of `beamdash.lua`

| Name | Default | Meaning |
|---|---|---|
| `ESP_IP` | `nil` | display IP; `nil` = the OutGauge address from the game menu |
| `ESP_PORT` | `4444` | UDP port – the same as OutGauge and the ETS2 plugin; the ESP recognises the game by the packet content |
| `RATE_HZ` | `30` | packets per second |

When both OutGauge and this mod send to port 4444, the display receives both packets (96 B and
100 B) and uses only the mod's; OutGauge is used again 1 s after the last mod packet. Disabling
OutGauge in the game menu saves some WiFi traffic.

## What is sent

Speed, RPM, **maximum and idle RPM of the engine**, gear and gear letter shown by the game,
**gearbox type** (manual, automatic, DCT, CVT, sequential, electric), throttle / brake / clutch,
fuel / tank capacity in litres, coolant and oil temperature, turbo boost, cruise control speed,
ambient temperature, ignition level, **vehicle model** (e.g. `etk800`), and bit flags: engine running,
ignition, parking brake, blinkers, hazard lights, low / high beam, fog lights, beacon, shift light,
ABS / TC / ESC active and present, turbo present, and the warnings for check engine, oil starvation,
coolant and oil overheating, radiator leak and low fuel. Field units are in the comments of
`beamng_packet.h`.

The packet starts with the bytes `BNGD` (`BEAM_PACKET_MAGIC`), then `version`, `gearboxType` and
`size`. Its size (100 B) differs from OutGauge (92/96 B) and the ETS2 `TruckPacket` (152 B).

BeamNG does not provide oil pressure – the built-in OutGauge sends 0 as well.

### Where the data comes from

Most values come from `electrics.values`, `damageTracker` and `wheels`, the same sources as the
[e90 CAN cluster protocol](https://github.com/veikkos/e90-can-cluster-beamng-protocol), a working
mod built the same way. These were not verified in the game yet and may need a fix after the first
test (check them in the vehicle Lua console with `dump(...)`):

| Value | Source |
|---|---|
| max / idle RPM | `powertrain.getDevice("mainEngine").maxRPM` / `.idleRPM` |
| gearbox type | `powertrain.getDevice("gearbox").type` |
| model | `v.config.mainPartName` |
| blinkers (blinking lamp) | `electrics.values.signal_L` / `signal_R` |
| hazard lights | `electrics.values.hazard_enabled` |
| low fuel | `electrics.values.lowfuel` |

A missing value does not break the packet – the field is sent as 0 or the bit stays cleared.

## Packet layout

The game builds the struct with normal C alignment (no `packed`); the fields are ordered so there is
no padding, so the packed struct in `beamng_packet.h` has the same layout. Both sides are
little-endian. `getStructDefinition()` in the Lua file and `beamng_packet.h` must always be changed
together.

## Testing without the ESP32

```bash
python3 tools/udp_dump.py        # listens on port 4444 and prints the packets
```

Set `ESP_IP` to the PC address and drive.

## Files

| File | Responsibility |
|---|---|
| [`mod/lua/vehicle/protocols/beamdash.lua`](mod/lua/vehicle/protocols/beamdash.lua) | the protocol: configuration, reading vehicle data, packet definition |
| [`src/beamng_packet.h`](src/beamng_packet.h) | packet layout – shared with the ESP32 firmware |
| [`Makefile`](Makefile) | `make` packs the mod into `build/beamdash.zip` |
| [`tools/udp_dump.py`](tools/udp_dump.py) | packet viewer for testing |
