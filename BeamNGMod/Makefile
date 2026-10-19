# Mod BeamNG.drive – pakowanie do zip.
#   make          -> build/beamdash.zip  (wrzucić do mods/ w folderze użytkownika BeamNG)
#   make clean

BUILD := build
SRC   := mod/lua/vehicle/protocols/beamdash.lua

.PHONY: all clean

all: $(BUILD)/beamdash.zip

$(BUILD)/beamdash.zip: $(SRC) | $(BUILD)
	rm -f $@
	cd mod && python3 -m zipfile -c ../$@ lua

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)