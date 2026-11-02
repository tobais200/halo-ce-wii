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
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES) cache_file_formats.o

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
	gcc -std=c11 -Wall -Wextra -I$(CURDIR)/third_party/OpenCE/port/linux/game -I$(CURDIR)/third_party/lz4 -I$(CURDIR)/source \
		$(CURDIR)/tools/host_check.c $(OPENCE_GEN) $(CURDIR)/third_party/lz4/lz4.c $(CURDIR)/source/xdvdfs.c \
		-o $(CURDIR)/build/host_check
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

#---------------------------------------------------------------------------------
%.jpg.o %_jpg.h : %.jpg
	@echo $(notdir $<)
	$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
