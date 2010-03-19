/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth input/output support
*/

#include "../nf-support.h"

#include <stdio.h>

NF_WEAK void nf_init_io ()
{
}

NF_WEAK uint8_t nf_getc ()
{
    return getchar ();
}

NF_WEAK void nf_putc (uint8_t c)
{
    putchar (c);
}
