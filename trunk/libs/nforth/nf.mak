LIBS += nforth
DESCRIPTION.nforth = The nForth language interpreter core
DIR.INCLUDE.C += :include/nforth

TARGETS.nforth = nforth$L
SRC.nforth$L = $(wildcard libs/nforth/*.c)
CFLAGS.nforth$L += -fno-toplevel-reorder
# -fno-align-functions

ifeq ($(ARCH),avr)
CFLAGS.nforth$L += -fomit-frame-pointer -finline-limit=2
endif

# Target-specific low-level functions
ifeq ($(TARGET),avr)
SRC.nforth$L += $(wildcard libs/nforth/$(TARGET)/*.c)
else
SRC.nforth$L += $(wildcard libs/nforth/ansi/*.c)
endif
