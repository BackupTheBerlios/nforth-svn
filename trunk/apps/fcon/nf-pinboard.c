/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Pinboard-specific nForth extensions
*/

#ifdef ARCH_AVR

#include "nf.h"

#include <util/delay.h>

/* This is a example of the so-called "application-specific nForth extension".
 * It provides additional FORTH words which allow manipulating your target hardware.
 * Try to avoid exposing very low-level functionality to FORTH application (like
 * bit-fiddling with ports): instead, for maximal effectivity provide "macro" words
 * which implement large pieces of functionality for your prototype hardware.
 *
 * This example assumes the following pinboard wiring:
 *
 * PA0 - connected to potentiometer (ADC mode)
 * PA1 - connected to CTS signal on FT232 (serial port flow control)
 * PA[6,7] - connected to LED cluster elements 0,1
 * PC[0..7] - connected to LED cluster elements 2-9
 * PD0,PD1 (RXD,TXD) - connected to FT232 (serial port)
 * PD[3-6] - connected to buttons A,B,C,D
 */

void nf_app_cts_on ()
{
    // FT232 uses inverted CTS signal
    PORTA &= ~_BV (1);
}

void nf_app_cts_off ()
{
    PORTA |= _BV (1);
}

// Enable or disable a LED (led number = 0..9)
// ( led state -- )
WORD_1st ("LED", LED, WHF_TYPE_NATIVE, nf_voc_core_inter - __CODE_BASE)
    uint8_t state = nf_pop ();
    uint8_t led = nf_pop ();
    uint16_t mask = ((uint16_t)1 << (led + 6));
    if ((uint8_t)mask)
        if (state)
            PORTA |= (uint8_t)mask;
        else
            PORTA &= ~((uint8_t)mask);
    else
        if (state)
            PORTC |= (uint8_t)(mask >> 8);
        else
            PORTC &= ~((uint8_t)(mask >> 8));
END_WORD

// Sleep for given number of milliseconds (1/1000 seconds)
// We can't use _delay_ms here since it needs a constant argument.
// ( ms -- )
WORD ("DELAY", DELAY, WHF_TYPE_NATIVE)
    uint16_t x = nf_pop ();
    while (x)
    {
        uint16_t y = x;
        // every loop is 4 clocks
        if (y >= ((65536 * 4000) / F_CPU))
            y = ((65536 * 4000) / F_CPU);
        x -= y;
        y *= (uint16_t) (F_CPU / 4000);
        _delay_loop_2 (y);
    }
END_WORD

// VOCABULARY()  must come before the definition of the last word in vocabulary
VOCABULARY (nf_voc_pinboard);

// Read the ADC and push the value to the stack
// ( -- val )
WORD ("ADC", _ADC, WHF_TYPE_NATIVE)
    ADCSRA |= _BV (ADSC);
    while (ADCSRA & _BV (ADSC))
        ;
    nf_push (ADCW);
END_WORD

NF_ALIGN_EOF

#endif // ARCH_AVR
