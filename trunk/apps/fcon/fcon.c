/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    A simple interactive nForth console
*/

#include "nf.h"
#include "nf-pinboard.h"

nf_context_t my_ctx;

void nf_app_clean ()
{
}

void nf_app_reset ()
{
    // Activate the previously allocated context
    nf_ctx = &my_ctx;
    nf_reset_ctx (&my_ctx);

    // Set up the core vocabulary
#ifdef ARCH_AVR
    my_ctx.voc [nfvCore] = NF_CODE2ADDR (&nf_voc_pinboard);
#else
    my_ctx.voc [nfvCore] = NF_CODE2ADDR (&nf_voc_core_inter);
#endif

    // Load the pointer to current context stack into the stack pointer
    nf_dsp = my_ctx.stack;

    nf_init_io ();
    nf_init_pinboard ();
}

int main (void)
{
    // First of all call either RESET() or CLEAN()
    // RESET() assumes that EEPROM contents has been preserved from a previous
    // session (which in the end called CLEAN sometimes long ago).
    // We'll call RESET and if Data Allocation Pointer is zero (which indicates
    // the MCU is uninitialized) will invoke CLEAN.
    RESET ();
    if (nf_global.dap == 0xffff)
        CLEAN (); // note that CLEAN calls RESET again

    nf_execute (MAIN);

    // MAIN will never return so we can save a few bytes by avoiding
    // the 'return' code
    for (;;) ;
}
