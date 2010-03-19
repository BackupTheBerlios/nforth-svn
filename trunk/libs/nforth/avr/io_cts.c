/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth system-specific support: UART input/output using polling
*/

#include "../nf-support.h"

// These functions can be overrided by application to provide a way
// to enable/disable the CTS signal (which tells the computer to
// pause data feeding).

__attribute__((weak)) void nf_app_cts_on ()
{
}

__attribute__((weak)) void nf_app_cts_off ()
{
}
