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
	@echo "port/wii: refusing to compile OpenCE into a Wii campaign."
	@echo "No PowerPC/GX device exists yet. See port/wii/TODO.md."
	@echo "The Homebrew Channel stub remains: make && make dist"
	@exit 2

clean:
	@rm -rf build
