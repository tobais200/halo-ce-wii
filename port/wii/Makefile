# Wii platform skeleton. Compiles port/wii/src only.
# `make opence` refuses: there is no campaign DOL.
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

CFLAGS	:= -g -O2 -Wall -Wextra $(MACHDEP) -I$(CURDIR)/include -I$(LIBOGC_INC)

.PHONY: all clean status opence

all: build/wii_platform.o

build/wii_platform.o: src/wii_platform.c include/wii_platform.h
	@mkdir -p build
	@echo wii_platform.c
	@$(CC) $(CFLAGS) -c $< -o $@

status:
	@echo "port/wii: platform skeleton (not a campaign)"
	@if [ -f ../../third_party/OpenCE/port/linux/port.json ]; then \
		echo "OpenCE submodule: present (not compiled)"; \
	else \
		echo "OpenCE submodule: missing"; \
		echo "  git submodule update --init third_party/OpenCE"; \
	fi
	@echo "playable campaign: no"

opence:
	$(MAKE) -C ../..
	@echo "OpenCE subset is linked into boot.dol."
	@echo "  cache_file_formats.c identify"
	@echo "  BSP geometry and bitmap/lightmap decode (xbox_textures.c rules)"
	@echo "  DeLa widget tags, string-list glyphs, event-handler flags"
	@echo "  hs_runtime.c special forms on the scenario script-node array"
	@echo "  game.c tick calls players, units, bipeds, weapons, ai, objects"
	@echo "  tag cache working set is streamed; the 23 MiB Xbox cache is not mapped"
	@echo "Bink, networking, the rasterizer, collisions.c, and game_engine.c are not compiled."

clean:
	@rm -rf build
