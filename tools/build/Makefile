# Build profiles (both share one build/ dir; switching profiles just
# reconfigures CMake and rebuilds, which takes seconds):
#   make        fast code      (-O2)
#   make debug  debug symbols  (-O0 -g3)
#
# Paths are resolved relative to the repository root, so this Makefile
# works from anywhere (it is included by the root Makefile).

SPEED ?= fast

ROOT     := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))../..)
BUILD    := $(ROOT)/build
CM       := $(ROOT)/tools/build

CMAKE = cmake -S $(CM) -B $(BUILD) -DCMAKE_TOOLCHAIN_FILE=$(CM)/toolchain.cmake -DPROFILE=$(SPEED)

.PHONY: all debug flash clean

all:
	$(CMAKE) && cmake --build $(BUILD)

debug:
	$(MAKE) SPEED=debug all

flash: all
	@test -f $(ROOT)/main.cpp || { echo "no main.cpp in repo root - library-only build, nothing to flash"; exit 1; }
	$(ROOT)/tools/build/upload.sh $(BUILD)/firmware.bin

clean:
	rm -rf $(BUILD)
