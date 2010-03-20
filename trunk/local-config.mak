AVR-GCC.MCU = atmega16
AVR-GCC.FLASHFLAGS = -D "-c stk500v2 -P /dev/stk500 -p m16"

# Clock frequency for your MCU
CFLAGS.DEF += -DF_CPU=8000000UL

# Choose the target platform here (avr, x86_64-linux, x86-linux)
TARG = x86_64-linux
# Choose compiling mode (release, debug)
MODE = release

define setup_x86_64-linux
ARCH = x86_64
TARGET = linux
# There's little sense in building a x86_64 nforth kernel without debug info
MODE = debug
endef

define setup_x86-linux
ARCH = x86
TARGET = linux
# Uncoment these to compile 32-bit code with the 64-bit gcc
#CFLAGS += -m32
#LDFLAGS += -m32
MODE = debug
endef

define setup_avr
ARCH = avr
TARGET = avr
MODE = release
endef

# Execute the respective setup
$(eval $(setup_$(TARG)))
