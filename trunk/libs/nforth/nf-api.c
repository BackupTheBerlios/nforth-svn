/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Interpreter core
*/

#include "nf-support.h"

NF_RAM nf_context_t *nf_ctx;
NF_RAM nf_global_state_t nf_global;
NF_EEPROM nf_global_state_t nf_global_save =
{
    // 0xffff means the state is uninitialized
    .dap = 0xffff,
};

#ifdef NF_GLOBAL_STATE
NF_RAM cell_t nf_state;
#endif

void nf_memcpy (addr_t dst, addr_t src, uint8_t n)
{
    while (n--)
    {
        nf_writeb (dst, nf_readb (src));
        src++;
        dst++;
    }
}

cell_t nf_strlen (addr_t str)
{
    addr_t cur = str;
    while (nf_readb (cur))
        cur++;
    return cur - str;
}

static inline void nf_memset (void *addr, uint8_t c, addr_t n)
{
    uint8_t *dest = (uint8_t *)addr;
    while (n--)
        *dest++ = c;
}

void nf_reset_ctx (nf_context_t *ctx)
{
    nf_memset (ctx, 0, sizeof (nf_context_t));

    // Reset user dictionary
    ctx->voc [nfvUser] = nf_global.last;
#ifndef NF_GLOBAL_STATE
    ctx->state = 0;
#endif
}
