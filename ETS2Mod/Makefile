# Wtyczka telemetrii ETS2/ATS – budowanie na Linuksie.
#   make          -> build/beamdash_telemetry.dll  (Windows x64, kompilator MinGW-w64)
#   make linux    -> build/beamdash_telemetry.so   (Linux x64)
#   make clean
# MinGW: sudo apt install g++-mingw-w64-x86-64

MINGW_CXX ?= x86_64-w64-mingw32-g++
CXX       ?= g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter -Isdk/include -Isrc
BUILD     := build
SRC       := src/plugin.cpp
DEPS      := $(SRC) src/truck_packet.h

.PHONY: all windows linux clean

all: windows

windows: $(BUILD)/beamdash_telemetry.dll

linux: $(BUILD)/beamdash_telemetry.so

# -static: .dll nie potrzebuje bibliotek MinGW na Windowsie
$(BUILD)/beamdash_telemetry.dll: $(DEPS) | $(BUILD)
	$(MINGW_CXX) $(CXXFLAGS) -shared -static -static-libgcc -static-libstdc++ -o $@ $(SRC) -lws2_32
	cp beamdash.ini $(BUILD)/

$(BUILD)/beamdash_telemetry.so: $(DEPS) | $(BUILD)
	$(CXX) $(CXXFLAGS) -shared -fPIC -fvisibility=hidden -o $@ $(SRC) -ldl
	cp beamdash.ini $(BUILD)/

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
