# This is a example config.mak which can be used if you don't want the
# configure script. In this case you just can put the preconfigured
# build options here (and possibly do some simple runtime detection
# using the GNU Make built-in features).

# Package name and version
CONF_PACKAGE = bach
CONF_VERSION = 0.0.1

# Default build mode
MODE = release

# Some helper functions
asciiup = $(shell echo $1 | tr a-z A-Z)
asciidown = $(shell echo $1 | tr A-Z a-z)

# Limited host runtime autodetection
ifndef HOST
ifneq ($(if $(ComSpec),$(ComSpec),$(COMSPEC)),)
HOST = win32
else
HOST = $(call asciidown,$(shell uname -s))
endif
endif

# Target operating system
TARGET = avr
# Target CPU/architecture
ARCH = avr

# Check if the respective tools are installed
MAKEDEP = $(shell which makedep 2>/dev/null)
DOXYGEN = $(shell which doxygen 2>/dev/null)

# Additional compilation flags
CXXFLAGS = -fno-exceptions -fno-rtti-fvisibility=hidden -Wno-non-virtual-dtor

# Since we have no config.h, we have to pass at least the arch and os to compiler
CFLAGS.DEF += -DARCH_$(call asciiup,$(ARCH)) -DTARGET_$(call asciiup,$(TARGET))
