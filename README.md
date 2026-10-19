# Lcd.BeamNG.drive

An external telemetry display for **BeamNG.drive** and **Euro Truck Simulator 2 / American Truck
Simulator**, built on an ESP32 with an ILI9341 screen. The game sends telemetry over the local
network (UDP) – BeamNG with its built-in **OutGauge** protocol or the [BeamNGMod](BeamNGMod)
protocol mod, ETS2/ATS through the [ETS2Mod](ETS2Mod) plugin – and the ESP32 recognises the game from the packet and shows speed,
RPM, gear, pedals and warning lights. An RGB LED works as a shift light.

```
BeamNG.drive ──UDP OutGauge──┐
BeamNG.drive ──UDP BeamNGMod─┤
ETS2 / ATS ────UDP ETS2Mod───┴─► WiFi ──► ESP32 ──SPI──► ILI9341 + XPT2046 touch
                                            ├──► RGB LED
                                            └──◄ 4 buttons
```

The on-screen texts are in Polish (default) or English – chosen on the configuration screen.
They have no diacritics, since the built-in TFT_eSPI fonts do not have them. This README quotes
the English texts.

## Hardware

- ESP32-S3 (default, board `4d_systems_esp32s3_gen4_r8n16`) or ESP32 WROOM-32
- ILI9341 320×240 SPI display with XPT2046 touch controller
- 4 push buttons (switching to GND)
- RGB LED + 3 resistors, 220–330 Ω

### Wiring

| Signal | ESP32-S3 | ESP32 WROOM |
|---|---|---|
| MOSI (SDI) / T_DIN | 11 | 23 |
| SCK / T_CLK | 12 | 18 |
| T_DO (touch MISO) – display SDO **not connected** | 13 | 19 |
| Display CS | 10 | 5 |
| DC | 9 | 16 |
| RST | 14 | 17 |
| LED (backlight) | 21 | 4 |
| T_CS (touch) | 8 | 21 |
| T_IRQ (touch) | 18 | 22 |
| Button 1 / 2 / 3 / 4 | 4 / 5 / 6 / 7 | 32 / 33 / 25 / 26 |
| LED R / G / B | 15 / 16 / 17 | 27 / 14 / 13 |

The touch controller shares the SPI bus with the display. Leave the display's SDO (MISO) pin
unconnected: on many ILI9341 modules it disturbs touch readings, and the display does not need
it. Only T_DO goes to MISO. All pins are defined in [`include/config.h`](include/config.h).
The LED type is set with `LED_COMMON_ANODE` (0 = common cathode, 1 = common anode).

## Building and uploading

PlatformIO project (Arduino framework). Environments in `platformio.ini`:

| Environment | Board |
|---|---|
| `esp32s3` (default) | ESP32-S3 4D Systems gen4 R8N16 |
| `esp32s3_ota` | ESP32-S3 uploaded over WiFi (see [OTA](#ota-uploading-over-wifi)) |
| `esp32dev` | ESP32 WROOM-32 (partitions `min_spiffs.csv`: 2 × 1.9 MB app, needed for OTA) |
| `esp32dev_ota` | ESP32 WROOM-32 uploaded over WiFi |
| `esp32-c3-devkitm-1` | ESP32-C3 – no pin set, does not compile |

```bash
pio run -t upload
```

```bash
pio device monitor
```

> **After every change to `config.h`, run Clean and then Build** (`pio run -t clean`).
> The file is injected into the TFT_eSPI library with the `-include` flag, and PlatformIO
> does not detect changes to it, so the library would not be rebuilt otherwise.

## OTA (uploading over WiFi)

With `OTA_ENABLE 1` in `config.h` (default) the firmware can be uploaded over WiFi.
OTA runs only when the device is connected to your network, not in hotspot mode.
The device announces itself as `beamng-dash.local` (`OTA_HOSTNAME`) and every upload needs the
password `OTA_PASSWORD` (default `BeamNG`), so it does not collide with other ESP32 boards with OTA
on the same network. The first upload with OTA enabled has to go over USB.

```bash
pio run -e esp32s3_ota -t upload    # ESP32-S3
pio run -e esp32dev_ota -t upload   # ESP32 WROOM
```

The `esp32s3_ota` environment in `platformio.ini` uses `upload_protocol = espota`,
`upload_port = beamng-dash.local` and `upload_flags = --auth=BeamNG`. If `.local` names do not
resolve on your PC, put the device IP there instead (shown on the "Waiting for game data" screen;
a fixed IP from the router is best). When you change `OTA_PASSWORD`, change `--auth` as well.
During the upload the screen shows "OTA update" with a progress bar and the LED
is purple; afterwards the device restarts. `OTA_ENABLE 0` removes the OTA code from the build.

## First start – WiFi

1. With no saved network, the device starts the **`BeamNG-Dash`** hotspot
   (password `beamng1234`); instructions are shown on the screen.
2. Connect to it with a phone – the setup page opens automatically
   (or go to `http://192.168.4.1`).
3. Choose a network, enter the password and save. The device restarts and connects.

Network credentials are stored in NVS and survive uploading new firmware.
If the connection fails within 15 s, the device starts the hotspot again.
To change the network, hold button 4 for 3 s (or use the configuration screen) – the
credentials are erased and the hotspot starts.

## Touch calibration

At startup the device checks whether a touch calibration is stored. If there is none:

1. Four crosshairs appear in the corners one at a time – touch each of them (a stylus works best).
   You have 15 s per corner.
2. Then a test: 4 random points, 5 s for each (countdown at the top). A touch counts
   if it lands within 15 px of the point.
3. Test passed → "Calibration OK", the data is saved in NVS and touch is enabled.
   Test failed (miss or no touch) → "Calibration failed", nothing is saved,
   touch is disabled and the device carries on starting up. Calibration runs again on the next start.

Calibration can also be run at any time from the configuration screen.
Timeouts and radius: `TOUCH_*` in `config.h`.

## BeamNG.drive setup

**Options → Other → OutGauge** (the *Protocols* tab in newer versions):

- enable OutGauge,
- **IP** – the address shown on the device screen ("Waiting for game data" – waiting for game data),
- **Port** – `4444`.

It is best to give the ESP32 a fixed IP address in your router.

Note: BeamNG fills only part of the OutGauge packet. The `car` field is always `"beam"`,
`display1` / `display2` are empty and `oilPressure` is always 0, and the packet does not
include the maximum RPM ([BeamNG protocol docs](https://docs.beamng.com/modding/protocols/)).

### BeamNGMod – more data than OutGauge (BeamNG v0.32+)

[`BeamNGMod/`](BeamNGMod) is a protocol mod for the game (`lua/vehicle/protocols/beamdash.lua`).
It sends `BeamPacket` – 100 bytes starting with `BNGD`
([`beamng_packet.h`](BeamNGMod/src/beamng_packet.h)) – to the same port 4444, with the real
maximum RPM of the engine, gearbox type, fuel tank capacity, the vehicle model and more lamps.
With the mod the RPM range is taken from the game instead of being learned. Build the zip with
`make` in `BeamNGMod/`, copy it to the game's `mods\` folder, details:
[BeamNGMod/README.md](BeamNGMod/README.md).

OutGauge can stay enabled: while mod packets arrive (and for 1 s after the last one) the display
ignores OutGauge packets, so the data does not jump between the two.

## Screens

Each game has its own list of screens and their order (see [Supported games](#supported-games)):
BeamNG shows screens 1–4, ETS2/ATS screens 1–5. Button **1** switches to the next screen of the
current game (after the last one it goes back to the first). After power-on, and when data from
another game arrives, the device starts on the first screen of that game.
On a car screen, touching the middle of the left edge goes to the previous screen and the
middle of the right edge to the next one (strips 80 px wide, y 60–179, nothing is drawn there;
sizes: `TOUCH_NAV_*` in `config.h`).

Button **2**, or touching the gear icon in the bottom-left corner of a car screen, opens the
**configuration screen** (button 2 also works on the "Waiting for game data" screen).

Button **4** held for 3 s (`BTN4_HOLD_MS` in `config.h`) erases the saved WiFi network and
restarts the device into hotspot mode for reconfiguration (not available on the configuration
screen – there WiFi is reset with the list option).

The device can be fully operated with only the buttons or only the touch screen: screen switching
and every configuration option work both ways. Exceptions: on the "Waiting for game data" and WiFi
hotspot screens the configuration screen opens only with button 2 (no gear icon there), and erasing
WiFi by touch is possible only through the configuration option. Touch calibration itself needs
touch; without a saved calibration touch stays off, and start-up waits 15 s for it (buttons cannot skip this).

### Configuration screen

A list of options; the highlighted option is orange. Button control:
**1** – run the highlighted option, **2** – previous, **3** – next, **4** – exit.
Touch: tapping an option runs it immediately, "Exit" in the top-right corner closes the menu.

- **"WiFi setup"** – asks "Yes/No" ("No" by default);
  "Yes" erases the network and restarts into hotspot mode.
- **"Touch calibration"** – the same procedure as on first start;
  a failed run keeps the previous calibration.
- **"Brightness"** – backlight 10–100% in 10% steps: 2/3 or "−"/"+",
  1 or "Save" stores it (the level also applies after a restart),
  4 or "Cancel" restores the previous level.
- **"Language: English"** – switches the language of all on-screen texts (Polish / English).
  It takes effect immediately and is stored in NVS (default: Polish).
- **"Factory reset"** – asks "Yes/No" ("No" by default); "Yes" erases all settings
  (WiFi network, touch calibration, brightness, language) and restarts the device, which then
  starts like a new one: touch calibration, then the WiFi hotspot.

The shift-light LED keeps working while the configuration screen is open.

**1. Race**
```
[=============== RPM bar ===============]
6500 rpm            < ABS TC HB HI >
 123   |    3    |  Fuel    45%
 km/h  |  (gear) |  Engine  90C
       |         |  Oil     95C
       |         |  Turbo   0.8
    Thr.  [==========              ]
    Brk.  [===                     ]
 ⚙  Clu.  [                        ]
```

**2. Analog gauges** – speedometer and tachometer with needles. OutGauge does not report the
car's top speed, so the speedometer scale starts at 0–160 km/h and steps up
(160 → 200 → 240 → 300 → 360 → 400) when the speed passes 95% of the scale; it goes back to 160
after a car change or a gap in the data, like the RPM range. Tachometer with a
red zone from 85% RPM, digital value and gear inside the dials, throttle and brake below.
The needles move smoothly towards the value.
```
        < ABS TC HB HI >
  ╭──────────╮   ╭──────────╮
 │   km/h  ╱  │ │  x1000 ╱   │
 │      ●     │ │      ●     │
 │     123    │ │      3     │
  ╰──────────╯   ╰──────────╯
 ⚙ Throttle [===]  Brake [==]
```

**3. Engine** – fuel, engine and oil temperature, oil pressure and turbo as bars
with values; above the warning threshold the colour turns orange, above the alarm threshold
red and blinking. `OIL` and `BAT` warning lamps.

**4. Pedals** – vertical throttle, brake and clutch bars in % and a scrolling graph
of throttle and brake over the last ~5 s (for practising smooth braking).

**5. Truck** (ETS2 / ATS only) – cruise control speed, speed limit (red when exceeded), brake air
pressure (orange when low, blinking red during emergency braking), AdBlue, retarder level, fuel
range, distance and time to destination, and lamps: handbrake, low beam, high beam, engine brake,
hazard lights, OIL, BAT.

Values a game does not send are shown as `--` (e.g. turbo in ETS2).

Common rules:

- shift light (from the game or > 95% RPM): the RPM bar / gear / dial blink red,
- gear: `R` (`R2`… in trucks) red, `N` green,
- RPM range: ETS2/ATS send the truck's maximum RPM and it is used directly; OutGauge does not,
  so for BeamNG the scale starts at 6000 and grows to the highest RPM seen in the current car;
  it resets after a car change (BeamNG: detected from the set of available dash lights and
  turbo, ETS2: truck brand, model and max RPM) and after every gap in the data,
- after 2 s without packets (pause, menu) the "Waiting for game data" screen comes back; it shows
  the setup hint of every supported game and the name of the last one.

RGB LED: blue – waiting for data; then off → green (50%) →
orange (70%) → red (85%) → blinking red with the shift light.

## Euro Truck Simulator 2 / American Truck Simulator

ETS2/ATS have no OutGauge, so [`ETS2Mod/`](ETS2Mod) adds a game plugin (SCS Telemetry SDK)
that sends truck data to the display:

- **own protocol** – `TruckPacket`, 152 bytes starting with `SCSD`
  ([`truck_packet.h`](ETS2Mod/src/truck_packet.h)), on the same UDP port 4444 as BeamNG;
  the ESP32 recognises the game by the packet,
- **more data than OutGauge** – real max RPM of the truck, cruise control, speed limit,
  air pressure, AdBlue, retarder, fuel range, navigation,
- **extra screen** – screen 5 "Truck" (only in ETS2/ATS), fixed 0–140 km/h speedometer.

Build on Linux with `make` (gives a Windows `.dll`), copy the `.dll` and `beamdash.ini` to the
game's `bin\win_x64\plugins\` and set the display IP in `beamdash.ini`. Details:
[ETS2Mod/README.md](ETS2Mod/README.md).

## Supported games

All games send to the same UDP port; the receiver recognises the game from the packet.
The list is in [`src/games/game_list.cpp`](src/games/game_list.cpp):

| Game | Packet | Screens | Options |
|---|---|---|---|
| BeamNG.drive (mod) | `BeamPacket` from BeamNGMod, 100 bytes, starts with `BNGD` | 1 Race, 2 Gauges, 3 Engine, 4 Pedals | – (max RPM from the game) |
| BeamNG.drive | OutGauge, 92 or 96 bytes – ignored while mod packets arrive | 1 Race, 2 Gauges, 3 Engine, 4 Pedals | `OPT_AUTO_RPM` – RPM range learned while driving |
| ETS2 | `TruckPacket`, 152 bytes, starts with `SCSD`, `game` = 1 | 1–4 + 5 Truck | `OPT_SPEED_140` – fixed 0–140 km/h speedometer |
| ATS | as ETS2, `game` = 2 | 1–4 + 5 Truck | `OPT_SPEED_140` |

Adding a game:

1. write a parser `bool parseXxx(const uint8_t* buf, size_t len, CarData& out)` in
   `src/games/xxx.*` – it checks the packet size / header and fills the shared
   [`CarData`](src/games/car_data.h) (units: km/h, bar, °C, 0..1; `NAN` = not available),
2. add one entry to `GAMES[]`: name, hint for the waiting screen (a text id `S_HINT_...` from
   [`src/lang.h`](src/lang.h), with a translation for every language), parser, screens in
   switching order, options.

The parsers are tried in list order and the first one that accepts the packet wins. A parser
returns `false` for a paused game, so the display goes back to the waiting screen.

## Code structure

| File | Responsibility |
|---|---|
| [`src/main.cpp`](src/main.cpp) | creates the objects and wires them together |
| [`src/wifi_setup.*`](src/wifi_setup.h) | `WifiSetup` – connects to the network or starts a hotspot with a web portal |
| [`src/games/game_list.cpp`](src/games/game_list.cpp) | list of supported games: parser, screens, options |
| [`src/games/game_def.h`](src/games/game_def.h) | `GameDef`, `ScreenId`, `OPT_*` |
| [`src/games/car_data.h`](src/games/car_data.h) | `CarData` – vehicle data shared by all games, `L_*` lamp bits |
| [`src/games/game_receiver.*`](src/games/game_receiver.h) | `GameReceiver` – UDP socket, recognises the game, emits `onTelemetry` / `onTelemetryLost` to a `TelemetryListener` |
| [`src/games/beamng_outgauge.*`](src/games/beamng_outgauge.h) | BeamNG parser (OutGauge, structure in [`src/outgauge.h`](src/outgauge.h)) |
| [`src/games/beamng_mod.*`](src/games/beamng_mod.h) | BeamNG mod parser (`BeamPacket` from [`BeamNGMod/src/beamng_packet.h`](BeamNGMod/src/beamng_packet.h)) |
| [`src/games/scs_telemetry.*`](src/games/scs_telemetry.h) | ETS2 / ATS parser (`TruckPacket` from [`ETS2Mod/src/truck_packet.h`](ETS2Mod/src/truck_packet.h)) |
| [`src/dashboard.*`](src/dashboard.h) | `Dashboard` – receives events, switches screens, info screens, LED |
| [`src/screens/screen.h`](src/screens/screen.h) | `Screen` interface (`enter` / `update` / `leave`) and `Telemetry` struct |
| [`src/screens/*_screen.*`](src/screens) | screens: `RaceScreen`, `GaugesScreen`, `EngineScreen`, `PedalsScreen`, `TruckScreen` |
| [`src/screens/widgets.*`](src/screens/widgets.h) | shared elements: colours, lamps, bars, gear, configuration icon |
| [`src/screens/config_screen.*`](src/screens/config_screen.h) | `ConfigScreen` – configuration screen (option list, WiFi, brightness, calibration) |
| [`src/backlight.*`](src/backlight.h) | `Backlight` – PWM backlight, level stored in NVS |
| [`src/lang.*`](src/lang.h) | on-screen texts in all languages (`LANG_STRINGS` table, `tr(S_...)`), language stored in NVS; a new language = a value in `Lang` and one more column in the table |
| [`src/in_receiver.*`](src/in_receiver.h) | `InReceiver` – reads the buttons, sends `PutButton1..4` and `PutButton4Long` events to `Dashboard` |
| [`src/touch.*`](src/touch.h) | `Touch` – XPT2046 touch: calibration with test, NVS storage, `enabled()` / `read()` / `recalibrate()` |
| [`src/ota.*`](src/ota.h) | `Ota` – uploading over WiFi (ArduinoOTA), enabled with `OTA_ENABLE` |
| [`src/rgb_led.*`](src/rgb_led.h) | `RgbLed` – PWM RGB LED |
| [`include/config.h`](include/config.h) | pins and settings (board selected with a `BOARD_*` flag) |

Event flow:

```
GameReceiver ─────onTelemetry / onTelemetryLost──► Dashboard ──update──► active Screen ──► LCD
InReceiver ───────PutButton1..4───────────────────► Dashboard ──────────► RGB LED
Touch ◄──────────────read() every 25 ms─────────── Dashboard ──────────► ConfigScreen
```

The screen refreshes at most ~30 times per second and redraws only the fields that changed
(sprites for animated elements), so it does not flicker. A screen creates its sprites
in `enter()` and frees them in `leave()`, so only the active screen's sprites are in memory.
New screen = new class derived from `Screen`, a `ScreenId` value, a case in `Dashboard::screen()`
and the id in the screen lists of the games that should show it.

## To do

- button 3 action outside the configuration screen (`Dashboard::PutButton3` is empty)
