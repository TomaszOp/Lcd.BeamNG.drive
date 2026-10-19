# ETS2Mod – telemetry plugin for Euro Truck Simulator 2 / American Truck Simulator

A plugin for the official **SCS Telemetry SDK**. The game loads it at start-up, and the plugin
sends the truck data over UDP (`TruckPacket`, 152 bytes, see [`src/truck_packet.h`](src/truck_packet.h))
to the Lcd.BeamNG.drive display or any other receiver.

> The ESP32 firmware does not understand this packet yet – recognising which game the data comes
> from is the next step.

## Building (on Linux)

```bash
sudo apt install g++-mingw-w64-x86-64   # once – cross-compiler for Windows
cd ETS2Mod
make            # build/beamdash_telemetry.dll  (Windows x64)
make linux      # build/beamdash_telemetry.so   (Linux x64)
```

The `.dll` is linked statically and needs no extra libraries on Windows (it imports only
`KERNEL32`, `msvcrt` and `WS2_32`). The SDK headers are in [`sdk/include`](sdk/include)
(SCS Telemetry SDK, MIT licence – [`sdk/LICENSE.txt`](sdk/LICENSE.txt)).

## Installing (Windows)

1. Copy `build/beamdash_telemetry.dll` **and** `build/beamdash.ini` to
   `...\steamapps\common\Euro Truck Simulator 2\bin\win_x64\plugins\`
   (create the `plugins` folder if it does not exist).
2. Set the display IP in `beamdash.ini` (see below).
3. Start the game. It shows a dialog about "Advanced SDK features" – confirm it (once per start).

The plugin writes to the game log (`Documents\Euro Truck Simulator 2\game.log.txt`),
lines starting with `[BeamDash]`, e.g. the target address and the truck it detected.
The same plugin works in American Truck Simulator (`bin\win_x64\plugins` in its folder).

## Configuration – `beamdash.ini`

| Key | Default | Meaning |
|---|---|---|
| `ip` | `255.255.255.255` | display IP; the default broadcast works without setting anything, but a fixed ESP32 address is more reliable |
| `port` | `4444` | UDP port – the same as OutGauge in BeamNG |
| `rate` | `30` | packets per second (1–100) |

After changing the file restart the game, or type `sdk reinit` in the game console.

## What is sent

Speed, RPM and **maximum RPM of the truck**, displayed gear and number of gears, cruise control
speed, navigation speed limit, fuel / capacity / range, AdBlue / capacity, brake air pressure,
brake / oil / water temperature, oil pressure, battery voltage, throttle / brake / clutch,
retarder level / steps, odometer, navigation distance and time, truck brand and model, and bit
flags: pause, engine, ignition, parking brake, engine brake, differential lock, blinkers, hazard
lights, parking / low / high beam, beacon, and the warnings for air pressure, fuel, AdBlue, oil,
water temperature and battery. Field units are in the comments of `truck_packet.h`.

The packet starts with the bytes `SCSD` (`TRUCK_PACKET_MAGIC`), then `version` and `game`
(1 = ETS2, 2 = ATS). When the game is paused the plugin keeps sending with the `TF_PAUSED` flag.

## Testing without the ESP32

```bash
python3 tools/udp_dump.py        # listens on port 4444 and prints the packets
```

Set `ip` in `beamdash.ini` to the PC address (or leave broadcast) and drive.

## Files

| File | Responsibility |
|---|---|
| [`src/plugin.cpp`](src/plugin.cpp) | the plugin: SDK registration, channel callbacks, `beamdash.ini`, UDP |
| [`src/truck_packet.h`](src/truck_packet.h) | packet layout – shared with the ESP32 firmware |
| [`beamdash.ini`](beamdash.ini) | default configuration, copied to `build/` |
| [`tools/udp_dump.py`](tools/udp_dump.py) | packet viewer for testing |
| [`sdk/`](sdk) | SCS Telemetry SDK headers |
