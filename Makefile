#---------------------------------------------------------------------------------
# Halo CE Wii engine stub. libogc / devkitPPC application.
# TARGET stays "boot" so the Homebrew Channel can launch boot.dol.
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/wii_rules

#---------------------------------------------------------------------------------
TARGET		:=	boot
BUILD		:=	build
SOURCES		:=	source third_party/lz4
DATA		:=	data
INCLUDES	:=	source third_party/lz4 third_party/OpenCE/port/linux/game
DISTDIR		:=	dist/apps/halo-ce
OPENCE_SRC	:=	$(CURDIR)/third_party/OpenCE/port/linux/game/cache_file_formats.c
OPENCE_GEN	:=	$(CURDIR)/build/cache_file_formats.c

#---------------------------------------------------------------------------------
CFLAGS		=	-g -O2 -Wall -Wextra $(MACHDEP) $(INCLUDE)
CXXFLAGS	=	$(CFLAGS)

LDFLAGS		=	-g $(MACHDEP) -Wl,-Map,$(notdir $@).map

# fat before wiiuse before bte before ogc: each library depends on the next.
LIBS		:=	-lfat -lwiiuse -lbte -logc -lm

LIBDIRS		:=

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(sFILES:.s=.o) $(SFILES:.S=.o)
export OPENCE_OBJS := game.o players.o cheats.o game_allegiance.o weapons.o projectiles.o equipment.o \
	bipeds.o units.o vehicles.o ai.o actors.o encounters.o actions.o path.o props.o \
	ai_communication.o ai_script.o objects.o object_types.o damage.o models.o \
	model_animations.o model_animation_definitions.o first_person_weapons.o memory_pool.o \
	collisions.o collision_bsp.o collision_models.o collision_features.o bsp2d.o bsp3d.o game_engine.o
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES) cache_file_formats.o hs_runtime.o data.o wii_hs_host.o wii_game_stubs.o wii_player_spawn.o wii_tag_cache.o wii_collision.o wii_effects.o wii_play.o wii_link_stubs.o $(OPENCE_OBJS)
export WII_ROOT := $(CURDIR)
export WII_HS_CFLAGS := -std=c99 -fno-strict-aliasing -DHALO_RELEASE \
	-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
	-I$(CURDIR)/port/wii/hs_shim \
	-I$(CURDIR)/third_party/OpenCE/source \
	-I$(CURDIR)/third_party/OpenCE/source/tag_files \
	-I$(CURDIR)/third_party/OpenCE/source/math \
	-I$(CURDIR)/third_party/OpenCE/source/cseries \
	-I$(CURDIR)/third_party/OpenCE/source/memory \
	-I$(CURDIR)/source

export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

export LIBPATHS	:=	-L$(LIBOGC_LIB) $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean dist host-check
.DEFAULT_GOAL := $(BUILD)

$(OPENCE_GEN): $(OPENCE_SRC) $(CURDIR)/tools/le_fourcc.py
	@mkdir -p $(CURDIR)/build
	python3 $(CURDIR)/tools/le_fourcc.py $(OPENCE_SRC) $(OPENCE_GEN)

#---------------------------------------------------------------------------------
$(BUILD): $(OPENCE_GEN)
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile
	@$(MAKE) --no-print-directory -C port/wii

#---------------------------------------------------------------------------------
host-check: $(OPENCE_GEN)
	@mkdir -p $(CURDIR)/build
	gcc -m32 -std=c99 -O2 -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-c $(CURDIR)/third_party/OpenCE/source/hs/hs_runtime.c -o $(CURDIR)/build/hs_runtime_host.o
	gcc -m32 -std=c99 -O2 -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-c $(CURDIR)/third_party/OpenCE/source/memory/data.c -o $(CURDIR)/build/data_host.o
	gcc -m32 -std=c99 -O2 -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-I$(CURDIR)/source \
		-c $(CURDIR)/port/wii/src/wii_hs_host.c -o $(CURDIR)/build/wii_hs_host_host.o
	@for obj in $(OPENCE_OBJS); do \
		python3 $(CURDIR)/tools/compile_opence.py "gcc -m32" $(CURDIR) $(CURDIR)/build $$obj || exit 1; \
	done
	gcc -m32 -std=c99 -O2 -fshort-wchar -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-I$(CURDIR)/source \
		-c $(CURDIR)/port/wii/src/wii_game_stubs.c -o $(CURDIR)/build/wii_game_stubs_host.o
	gcc -m32 -std=c99 -O2 -fshort-wchar -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-include $(CURDIR)/port/wii/include/wii_sys.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/port/wii/include \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-I$(CURDIR)/source \
		-c $(CURDIR)/port/wii/src/wii_player_spawn.c -o $(CURDIR)/build/wii_player_spawn_host.o
	gcc -m32 -std=c99 -O2 -fshort-wchar -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-include $(CURDIR)/port/wii/include/wii_sys.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/port/wii/include \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/tag_files \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-I$(CURDIR)/third_party/OpenCE/source/memory \
		-I$(CURDIR)/source \
		-c $(CURDIR)/port/wii/src/wii_tag_cache.c -o $(CURDIR)/build/wii_tag_cache_host.o
	python3 $(CURDIR)/tools/compile_opence.py "gcc -m32" $(CURDIR) $(CURDIR)/build wii_collision_host.o
	python3 $(CURDIR)/tools/compile_opence.py "gcc -m32" $(CURDIR) $(CURDIR)/build wii_effects_host.o
	python3 $(CURDIR)/tools/compile_opence.py "gcc -m32" $(CURDIR) $(CURDIR)/build wii_play_host.o
	gcc -m32 -std=c99 -O2 -fshort-wchar -Wall -Wextra -fno-strict-aliasing -DHALO_RELEASE \
		-include $(CURDIR)/port/wii/hs_shim/wii_hs_prefix.h \
		-I$(CURDIR)/port/wii/hs_shim \
		-I$(CURDIR)/third_party/OpenCE/source \
		-I$(CURDIR)/third_party/OpenCE/source/math \
		-I$(CURDIR)/third_party/OpenCE/source/cseries \
		-c $(CURDIR)/port/wii/src/wii_link_stubs.c -o $(CURDIR)/build/wii_link_stubs_host.o
	gcc -m32 -std=c11 -Wall -Wextra -I$(CURDIR)/third_party/OpenCE/port/linux/game -I$(CURDIR)/third_party/lz4 -I$(CURDIR)/source \
		$(CURDIR)/tools/host_check.c $(OPENCE_GEN) $(CURDIR)/third_party/lz4/lz4.c $(CURDIR)/source/xdvdfs.c $(CURDIR)/source/bsp_preview.c $(CURDIR)/source/wii_tex.c $(CURDIR)/source/wii_widget.c $(CURDIR)/source/wii_sim.c \
		$(CURDIR)/build/hs_runtime_host.o $(CURDIR)/build/data_host.o $(CURDIR)/build/wii_hs_host_host.o \
		$(CURDIR)/build/wii_game_stubs_host.o $(CURDIR)/build/wii_player_spawn_host.o $(CURDIR)/build/wii_tag_cache_host.o $(CURDIR)/build/wii_collision_host.o $(CURDIR)/build/wii_effects_host.o $(CURDIR)/build/wii_play_host.o $(CURDIR)/build/wii_link_stubs_host.o \
		$(addprefix $(CURDIR)/build/,$(OPENCE_OBJS)) \
		-lm -o $(CURDIR)/build/host_check
	$(CURDIR)/build/host_check

#---------------------------------------------------------------------------------
# Homebrew Channel layout: dist/apps/halo-ce/{boot.dol,boot.elf,meta.xml,icon.png}
dist: $(BUILD)
	@test -f $(TARGET).dol
	@test -f $(TARGET).elf
	@rm -rf $(DISTDIR)
	@mkdir -p $(DISTDIR)
	@cp -f $(TARGET).dol $(TARGET).elf meta.xml icon.png $(DISTDIR)/
	@echo "HBC layout: $(DISTDIR)/"

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol $(OUTPUT).map dist
	@$(MAKE) --no-print-directory -C port/wii clean

#---------------------------------------------------------------------------------
run:
	wiiload $(TARGET).dol

include $(CURDIR)/port/wii/hooks.mk

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
$(OUTPUT).dol: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

$(OFILES_SOURCES) : $(HFILES)

WII_HS_COMPILE = $(CC) -MMD -MP $(CFLAGS) $(WII_HS_CFLAGS)

hs_runtime.o: $(WII_ROOT)/third_party/OpenCE/source/hs/hs_runtime.c
	$(WII_HS_COMPILE) -MF $(DEPSDIR)/hs_runtime.d -c $< -o $@

data.o: $(WII_ROOT)/third_party/OpenCE/source/memory/data.c
	$(WII_HS_COMPILE) -MF $(DEPSDIR)/data.d -c $< -o $@

wii_hs_host.o: $(WII_ROOT)/port/wii/src/wii_hs_host.c
	$(WII_HS_COMPILE) -MF $(DEPSDIR)/wii_hs_host.d -c $< -o $@

$(OPENCE_OBJS):
	python3 $(WII_ROOT)/tools/compile_opence.py "$(CC)" $(WII_ROOT) . $@

wii_game_stubs.o: $(WII_ROOT)/port/wii/src/wii_game_stubs.c
	$(CC) -MMD -MP -fshort-wchar $(CFLAGS) $(WII_HS_CFLAGS) -MF $(DEPSDIR)/wii_game_stubs.d -c $< -o $@

wii_player_spawn.o: $(WII_ROOT)/port/wii/src/wii_player_spawn.c
	$(CC) -MMD -MP -fshort-wchar $(CFLAGS) $(WII_HS_CFLAGS) -include $(WII_ROOT)/port/wii/include/wii_sys.h -MF $(DEPSDIR)/wii_player_spawn.d -c $< -o $@

wii_tag_cache.o: $(WII_ROOT)/port/wii/src/wii_tag_cache.c
	$(CC) -MMD -MP -fshort-wchar $(CFLAGS) $(WII_HS_CFLAGS) -include $(WII_ROOT)/port/wii/include/wii_sys.h -MF $(DEPSDIR)/wii_tag_cache.d -c $< -o $@

wii_collision.o: $(WII_ROOT)/port/wii/src/wii_collision.c
	python3 $(WII_ROOT)/tools/compile_opence.py "$(CC) -DGEKKO" $(WII_ROOT) . wii_collision.o

wii_effects.o: $(WII_ROOT)/port/wii/src/wii_effects.c
	python3 $(WII_ROOT)/tools/compile_opence.py "$(CC) -DGEKKO" $(WII_ROOT) . wii_effects.o

wii_play.o: $(WII_ROOT)/port/wii/src/wii_play.c
	python3 $(WII_ROOT)/tools/compile_opence.py "$(CC) -DGEKKO" $(WII_ROOT) . wii_play.o

wii_link_stubs.o: $(WII_ROOT)/port/wii/src/wii_link_stubs.c
	$(CC) -MMD -MP -fshort-wchar $(CFLAGS) $(WII_HS_CFLAGS) -MF $(DEPSDIR)/wii_link_stubs.d -c $< -o $@

#---------------------------------------------------------------------------------
%.jpg.o %_jpg.h : %.jpg
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
