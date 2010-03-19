# tibs macros for avr-gcc toolkit

.SUFFIXES: .c .cpp .o .lo .a .pc .pc.in .out

ifndef AVR-GCC.MCU
$(error Please set the AVR-GCC.MCU variable to target$(NL)\
microcontroller identifier (see -mmcu= avr-gcc option description))
endif

AVR-GCC.CC ?= avr-gcc -c
AVR-GCC.CFLAGS ?= -pipe -Wall -Wextra -mmcu=$(AVR-GCC.MCU) \
    $(AVR-GCC.CFLAGS.$(MODE)) $(AVR-GCC.CFLAGS.DEF) $(AVR-GCC.CFLAGS.INC) $(CFLAGS)
AVR-GCC.CFLAGS.DEF = $(CFLAGS.DEF)
AVR-GCC.CFLAGS.INC = $(if $(DIR.INCLUDE.C),-I$(subst :, -I,$(DIR.INCLUDE.C)))

AVR-GCC.CFLAGS.release ?= -g -Os
AVR-GCC.CFLAGS.debug ?= -g -D__DEBUG__

AVR-GCC.CXX.OK := $(shell which avr-g++ 2>/dev/null)

ifneq ($(AVR-GCC.CXX.OK),)
AVR-GCC.CXX ?= avr-g++ -c
AVR-GCC.CXXFLAGS ?= $(AVR-GCC.CFLAGS) $(CXXFLAGS)
else
AVR-GCC.CXX ?= echo "C++ compiler is not installed"; false
endif

AVR-GCC.CPP ?= avr-gcc -E
AVR-GCC.CPPFLAGS ?= -pipe -x c-header $(AVR-GCC.CFLAGS.DEF) $(AVR-GCC.CFLAGS.INC)

AVR-GCC.LD ?= avr-gcc
AVR-GCC.LDFLAGS ?= -pipe -mmcu=$(AVR-GCC.MCU) \
    $(AVR-GCC.LDFLAGS.$(MODE)) $(LDFLAGS)
AVR-GCC.LDFLAGS.LIBS ?= $(LDLIBS)

AVR-GCC.LDFLAGS.release ?= -g
AVR-GCC.LDFLAGS.debug ?= -g

AVR-GCC.LINKLIB = $(if $(findstring $L,$1),,$(if $(findstring /,$1),$1,-l$1))

AVR-GCC.MDEP ?= $(or $(MAKEDEP),makedep)
AVR-GCC.MDEPFLAGS ?= -c -a -p'$$(OUT)' $(AVR-GCC.CFLAGS.DEF) $(AVR-GCC.CFLAGS.INC) $(MDEPFLAGS)

AVR-GCC.AR ?= avr-ar
AVR-GCC.ARFLAGS ?= crs

# Arbitrary assumptions about the programmer options
# Most likely user will have to override this
AVR-GCC.FLASHFLAGS ?= -c avrisp2 -P /dev/ttyUSB0 -p m168

# Translate application/library pseudo-name into an actual file name
XFNAME.AVR-GCC = $(addprefix $$(OUT),\
    $(patsubst %$E,%.out,\
    $(if $(findstring $L,$1),$(addprefix lib,$(patsubst %$L,%.a,$1)),$1)\
))

MKDEPS.AVR-GCC = \
    $(patsubst %.c,%.o,\
    $(patsubst %.cpp,%.o,\
    $(patsubst %.asm,%.o,\
    $(patsubst %.S,%.o,\
    $(call MKDEPS.DEFAULT,$1)))))

COMPILE.AVR-GCC.CXX  = $(AVR-GCC.CXX) -o $@ $(strip $(AVR-GCC.CXXFLAGS) $1) $<
COMPILE.AVR-GCC.CC   = $(AVR-GCC.CC) -o $@ $(strip $(AVR-GCC.CFLAGS) $1) $<

# Compilation rules ($1 = source file list, $2 = source file directories,
# $3 = module name, $4 = target name)
define MKCRULES.AVR-GCC
$(if $(filter %.c,$1),$(foreach z,$2,
$(addsuffix %.o,$(addprefix $$(OUT),$z)): $(addsuffix %.c,$z)
	$(if $V,,@echo COMPILE.AVR-GCC.CC $$< &&)$$(call COMPILE.AVR-GCC.CC,$(CFLAGS.$3) $(CFLAGS.$4) $(call .SYSLIBS,CFLAGS,$3,$4))))
$(if $(filter %.cpp,$1),$(foreach z,$2,
$(addsuffix %.o,$(addprefix $$(OUT),$z)): $(addsuffix %.cpp,$z)
	$(if $V,,@echo COMPILE.AVR-GCC.CXX $$< &&)$$(call COMPILE.AVR-GCC.CXX,$(CXXFLAGS.$3) $(CXXFLAGS.$4) $(call .SYSLIBS,CFLAGS,$3,$4))))
$(AVR-GCC.EXTRA.MKCRULES)
endef

LINK.AVR-GCC.AR = $(AVR-GCC.AR) $(AVR-GCC.ARFLAGS) $@ $^
define LINK.AVR-GCC.EXEC
    $(AVR-GCC.LD) -o $@ $(AVR-GCC.LDFLAGS) $(LDFLAGS) $1 $^ $(AVR-GCC.LDFLAGS.LIBS) $(LDFLAGS.LIBS) $2 -Wl,-Map,$(@:.out=.map)
    size $@
endef

# Linking rules ($1 = target full filename, $2 = dependency list,
# $3 = module name, $4 = unexpanded target name)
define MKLRULES.AVR-GCC
$1: $2\
$(if $(findstring $L,$4),
	$(if $V,,@echo LINK.AVR-GCC.AR $$@ &&)$$(LINK.AVR-GCC.AR))\
$(if $(findstring $E,$4),
	$(if $V,,@echo LINK.AVR-GCC.EXEC $$@ &&)$$(call LINK.AVR-GCC.EXEC,$(subst $(COMMA),$$(COMMA),$(LDFLAGS.$3) $(LDFLAGS.$4)) $(call .SYSLIBS,LDLIBS,$3,$4),$(foreach z,$(LIBS.$3) $(LIBS.$4),$(call AVR-GCC.LINKLIB,$z))))
$(AVR-GCC.EXTRA.MKLRULES)
endef

# Install rules ($1 = module name, $2 = unexpanded target file,
# $3 = full target file name)
define MKIRULES.AVR-GCC
$(if $(findstring $L,$2),\
$(foreach _,$3,
	$(if $V,,@echo INSTALL $_ to $(call .INSTDIR,$1,$2,LIB,$(CONF_LIBDIR)) &&)\
	$$(call INSTALL,$_,$(call .INSTDIR,$1,$2,LIB,$(CONF_LIBDIR)),0644)))\
$(if $(findstring $E,$2),
	$(if $V,,@echo INSTALL $3 to $(call .INSTDIR,$1,$2,BIN,$(CONF_BINDIR)) &&)\
	$$(call INSTALL,$3,$(call .INSTDIR,$1,$2,BIN,$(CONF_BINDIR)),0755))\
$(if $(INSTALL.INCLUDE.$2),
	$(if $V,,@echo INSTALL $(INSTALL.INCLUDE.$2) to $(call .INSTDIR,$1,$2,INCLUDE,$(CONF_INCLUDEDIR)) &&)\
	$$(call INSTALL,$(INSTALL.INCLUDE.$2),$(call .INSTDIR,$1,$2,INCLUDE,$(CONF_INCLUDEDIR)),0644))
endef

define MAKEDEP.AVR-GCC
	$(call RM,$@)
	$(if $(filter %.c,$^),$(AVR-GCC.MDEP) $(strip $(AVR-GCC.MDEPFLAGS) $(filter -D%,$1) $(filter -I%,$1)) -f $@ $(filter %.c,$^))
	$(if $(filter %.cpp,$^),$(AVR-GCC.MDEP) $(strip $(AVR-GCC.MDEPFLAGS) $(filter -D%,$2) $(filter -I%,$2)) -f $@ $(filter %.cpp,$^))
endef

# Dependency rules ($1 = dependency file, $2 = source file list,
# $3 = module name, $4 = target name)
define MKDRULES.AVR-GCC
$1: $(MAKEDEP_DEP) $2
	$(if $V,,@echo MAKEDEP.AVR-GCC $$@ &&)$$(call MAKEDEP.AVR-GCC,$(subst $(COMMA),$$(COMMA),$(CFLAGS.$3) $(CFLAGS.$4) $(CFLAGS)),$(subst $(COMMA),$$(COMMA),-D__cplusplus $(CXXFLAGS.$3) $(CXXFLAGS.$4) $(CXXFLAGS)))
$(AVR-GCC.EXTRA.MKDRULES)
endef

AVR-GCC.FLASH=$(DIR.TIBS)/extra/flash-elf.awk "$1" $(AVR-GCC.FLASHFLAGS) $2

# Flashing rules ($1 = module name, $2 = unexpanded target file,
# $3 = full target file name)
define MKFRULES.AVR-GCC
$(if $(findstring $E,$2),
	$(if $V,,@echo FLASH.AVR-GCC $3 &&)\
	$$(call AVR-GCC.FLASH,$3,$(strip $(FLASHFLAGS.$1) $(FLASHFLAGS.$2))))
endef
