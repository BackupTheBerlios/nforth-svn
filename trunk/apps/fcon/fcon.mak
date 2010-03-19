APPS += fcon
DESCRIPTION.fcon = A interactive nForth console
FLASH.TARGETS += fcon

TARGETS.fcon = fcon$E
SRC.fcon$E = $(wildcard apps/fcon/*.c)
LIBS.fcon$E = nforth$L

# ALWAYS DEFINE THIS FLAG FOR FORTH CODE!!!
CFLAGS.fcon = -fno-toplevel-reorder

ifeq ($(findstring /$(ARCH)/,/avr/),)
# Don't forget to put the RAM and EEPROM sections at the address
# specified by the __EEPROM_BASE and __RAM_BASE macros in nf-defs.h!
LDFLAGS.fcon += -Wl,--script=libs/nforth/ldscripts/$(ARCH)-$(TARGET)
endif

# Exclude eeprom from flashing
FLASHFLAGS.fcon = -X eeprom
