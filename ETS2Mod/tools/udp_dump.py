#!/usr/bin/env python3
"""Podgląd pakietów TruckPacket na PC – test wtyczki bez ESP32.

  python3 tools/udp_dump.py [port]

Wtyczka musi wysyłać na IP tego komputera (albo broadcast).
"""
import socket
import struct
import sys

FMT = "<4sBBHII5f bBBBB3x 5f 6f 3f 3f 16s24s"
NAMES = ("magic version game size counter flags speed rpm rpmMax cruiseSpeed speedLimit "
         "gear gearsForward gearsReverse retarder retarderSteps fuel fuelCapacity fuelRange adblue "
         "adblueCapacity airPressure brakeTemp oilTemp oilPressure waterTemp batteryVoltage "
         "throttle brake clutch odometer navDistance navTime brand name").split()
assert struct.calcsize(FMT) == 152

port = int(sys.argv[1]) if len(sys.argv) > 1 else 4444
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.bind(("", port))
print(f"Nasluchuje na porcie {port}...")
while True:
    data, addr = sock.recvfrom(2048)
    if len(data) != 152 or data[:4] != b"SCSD":
        print(f"{addr[0]}: inny pakiet, {len(data)} B")
        continue
    p = dict(zip(NAMES, struct.unpack(FMT, data)))
    print(f"#{p['counter']:6d} {p['speed'] * 3.6:6.1f} km/h {p['rpm']:5.0f}/{p['rpmMax']:.0f} rpm "
          f"bieg {p['gear']:3d}/{p['gearsForward']} paliwo {p['fuel']:.0f}/{p['fuelCapacity']:.0f} l "
          f"flagi 0x{p['flags']:08x} {p['brand'].rstrip(b'\0').decode()} {p['name'].rstrip(b'\0').decode()}")
