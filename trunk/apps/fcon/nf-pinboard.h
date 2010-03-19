/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Pinboard-specific functions for nForth
*/

#ifndef __NF_PINBOARD_H__
#define __NF_PINBOARD_H__

// For non-AVR platforms we define empty stubs

#if defined ARCH_AVR

DECLARE_VOCABULARY (nf_voc_pinboard);

static inline void nf_init_pinboard ()
{
    // Disable JTAG (if JTAGEN fuse is enabled).
    // This code is not needed if you disable JTAG with fuses.
    // According to datasheet, JTD must be set twice within 4 clocks.
    MCUCSR |= _BV (JTD);
    MCUCSR |= _BV (JTD);

    /* Set up ports PC0-PC7, PA1,PA6-PA7 for output */
    DDRA = 0xc2;
    DDRC = 0xff;
    DDRD = _BV (4) | _BV (5);

    /* Set up ADC for reading from port A0 at lowest possible rate */
    ADCSRA = _BV (ADEN) | _BV (ADPS2) | _BV (ADPS1) | _BV (ADPS0);
    ADMUX = _BV (REFS1) | _BV (REFS0);
}

#else

static inline void nf_init_pinboard ()
{ }

#endif

#endif /* __NF_PINBOARD_H__ */
