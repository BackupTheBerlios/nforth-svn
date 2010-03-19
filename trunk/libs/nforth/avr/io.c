/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth system-specific support: UART input/output using polling
*/

#include "../nf-support.h"

/* The i/o functions are marked weak so that user may provide
 * its own functions which will override the code from this file.
 */

extern void nf_app_cts_on ();
extern void nf_app_cts_off ();

__attribute__((weak)) void nf_putc (uint8_t c)
{
    if (c == '\n')
        nf_putc ('\r');
    loop_until_bit_is_set (UCSRA, UDRE); 
    UDR = c; 
}

__attribute__((weak)) uint8_t nf_getc ()
{
    // Tell sender to go ahead
    nf_app_cts_on ();

    loop_until_bit_is_set (UCSRA, RXC);
    uint8_t c = UDR;

    // Pause sender until we're ready to receive again
    nf_app_cts_off ();

    // Translate CR to traditional NL
    if (c == '\r')
        c = '\n';

    return c;
}

#ifdef NF_TRACE
#include <stdio.h>

static int stdio_putchar (char c, FILE *stream)
{ (void)stream; nf_putc (c); return 0; }

static FILE nf_stdout = FDEV_SETUP_STREAM (stdio_putchar, NULL, _FDEV_SETUP_WRITE);
#endif

__attribute__((weak)) void nf_init_io ()
{
#define BAUD 115200
#include <util/setbaud.h>
    UBRRH = UBRRH_VALUE;
    UBRRL = UBRRL_VALUE;
#if USE_2X
    UCSRA |= (1 << U2X);
#else
    UCSRA &= ~(1 << U2X);
#endif
    UCSRB = (1 << RXEN) | (1 << TXEN);
    // 7E1
    UCSRC = (1 << URSEL) | (1 << UPM1) | (1 << UCSZ1);
    // 8N1
    //UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
#ifdef NF_TRACE
    stdout = &nf_stdout;
#endif
}
