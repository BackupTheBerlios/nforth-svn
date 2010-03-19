/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth support for word tracing (debugging)
*/

#include "nf-support.h"

#ifdef NF_TRACE

#include <stdio.h>

int nf_trace_level;

void nf_trace_word (const char *comment, addr_t word)
{
    // Keep the number of local variables low, so that gcc can avoid using
    // a local stack frame for local variables, thus trashing the Y register
    // on AVR (which we're using for nf_dsp).

    uint8_t i;
    for (i = 0; i < nf_trace_level; i++)
      putc (' ', stdout);

    if (word >= 0xc000)
    {
        if (word < 0xc200)               // JUMP
            printf ("%s: JMP %d", comment, (cell_t)(word - 0xc100));
        else if (word < 0xc400)          // JZ
            printf ("%s: JZ %d", comment, (cell_t)(word - 0xc300));
        else                            // NUM
            printf ("%s: PUSH %d", comment, (cell_t)(word - 0xe200));
    }
    else
    {
        fputs (comment, stdout);
        fputs (": ", stdout);

        word += MEMBER_OFFSET (nf_word_t, flags);
        uint8_t flags = nf_readb (word);

        const char *wtype;
        switch (flags & WHF_TYPE_MASK)
        {
            case WHF_TYPE_DATA:
                wtype = "DATA";
                break;
            case WHF_TYPE_CODE:
                wtype = "CODE";
                break;
            case WHF_TYPE_NATIVE:
                wtype = "CALL";
                break;
            case WHF_TYPE_DOES:
            default:
                wtype = "DOES";
                break;
        }
        fputs (wtype, stdout);
        putc (' ', stdout);

        flags = (flags & WHF_LENGTH_MASK) + 1;
        for (; flags != 0; flags--)
            putc (nf_readb (++word), stdout);
    }

    // Now print out the stack
    printf ("\t\t");
    if (nf_ctx)
    {
        cell_t *cur;
        for (cur = nf_ctx->stack; cur < nf_dsp; cur++)
            printf ("%d ", *cur);
    }
    printf ("\n");
}

#endif
