#!/usr/bin/env python3
"""Podgląd pakietów BeamPacket na PC – test modu BeamNG bez ESP32.

  python3 tools/udp_dump.py [port]

Mod musi wysyłać na IP tego komputera (ESP_IP w beamdash.lua albo adres OutGauge w menu gry).
"""
import socket
import struct
import sys

FMT = "<4sBBHII14fbcBB24s"
NAMES = ("magic version gearboxType size counter flags speed rpm rpmMax rpmIdle throttle brake clutch "
         "fuel fuelCapacity engTemp oilTemp turbo cruiseSpeed envTemp gear gearMode ignition reserved "
         "model").split()
assert struct.calcsize(FMT) == 100
GEARBOX = ["?", "manual", "auto", "dct", "cvt", "seq", "electric"]

port = int(sys.argv[1]) if len(sys.argv) > 1 else 4444
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("", port))
print(f"Nasluchuje na porcie {port}...")
while True:
    data, addr = sock.recvfrom(2048)
    if len(data) != 100 or data[:4] != b"BNGD":
        print(f"{addr[0]}: inny pakiet, {len(data)} B")
        continue
    p = dict(zip(NAMES, struct.unpack(FMT, data)))
    model = p["model"].split(b"\0")[0].decode(errors="replace")
    gb = GEARBOX[p["gearboxType"]] if p["gearboxType"] < len(GEARBOX) else "?"
    print(f"#{p['counter']:6d} {p['speed'] * 3.6:6.1f} km/h {p['rpm']:5.0f}/{p['rpmMax']:.0f} rpm "
          f"bieg {p['gear']:3d} {p['gearMode'].decode()} ({gb}) paliwo {p['fuel']:.0f}/{p['fuelCapacity']:.0f} l "
          f"woda {p['engTemp']:.0f} olej {p['oilTemp']:.0f} C flagi 0x{p['flags']:08x} "
          f"{model}")
