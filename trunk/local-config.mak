AVR-GCC.MCU = atmega16
AVR-GCC.FLASHFLAGS = -D "-c stk500v2 -P /dev/stk500 -p m16"

# Clock frequency for your MCU
CFLAGS.DEF += -DF_CPU=8000000UL

# Choose the target platform here (avr, x86_64-linux, x86-linux)
TARG = avr
MODE = release

define setup_x86_64-linux
ARCH = x86_64
TARGET = linux
endef

define setup_x86-linux
ARCH = x86
TARGET = linux
CFLAGS += -m32
LDFLAGS += -m32
endef

define setup_avr
ARCH = avr
TARGET = avr
MODE = release
endef

# Execute the respective setup
$(eval $(setup_$(TARG)))
