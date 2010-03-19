/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Interpreter core
*/

#include "nf-support.h"

#include <stdlib.h>
#include <string.h>

// This flag is set when WORD encounters an '\n'
NF_RAM uint16_t nf_word_eol;

//---// Unnamed words, never called by user directly //---//

// Check-Compile-Only, call THROW if interpret mode
WORD_CODE_1st ("\\0", _CCO, WHF_TYPE_CODE, _NF_ASMNAME (nf_voc_core_io) - __CODE_BASE)
    CODE_WORD (STATE)
    CODE_WORD (_AT)
    CODE_JZ (compile_only)
        CODE_EXIT
    CODE_LABEL (compile_only)
        CODE_PUSH_NUM (NFE_COMPILE_ONLY)
        CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("\\0", _DOES, WHF_TYPE_CODE)
    CODE_WORD (LAST)            // &last
    CODE_WORD (_AT)             // last
    CODE_PUSH_NUM (2)           // last, MEMBER_OFFSET(nf_word_t,flags)
    CODE_WORD (ADD)             // &last->flags
    CODE_WORD (DC_AT)           // &last->flags, flags
    CODE_WORD (DUP)             // &last->flags, flags, flags
    CODE_PUSH_NUM (WHF_TYPE_MASK)// &last->flags, flags, flags, WHF_TYPE_MASK
    CODE_WORD (AND)             // &last->flags, flags, flags&WHF_TYPE_MASK
    CODE_PUSH_NUM (WHF_TYPE_DATA)// &last->flags, flags, flags&WHF_TYPE_MASK, WHF_TYPE_DATA
    CODE_WORD (NEQ)             // &last->flags, flags, flags&WHF_TYPE_MASK==WHF_TYPE_DATA
    CODE_JZ (ok)
        CODE_PUSH_NUM (NFE_DOES_WRONG)
        CODE_WORD (THROW)
    CODE_LABEL (ok)             // &last->flags, flags
        CODE_PUSH_NUM (0xff ^ WHF_TYPE_MASK)
        CODE_WORD (AND)         // &last->flags, flags&~WHF_TYPE_MASK
        CODE_PUSH_NUM (WHF_TYPE_DOES)
        CODE_WORD (OR)          // &last->flags, (flags&~WHF_TYPE_MASK)|WHF_TYPE_DOES
        CODE_WORD (SWAP)        // (flags&~WHF_TYPE_MASK)|WHF_TYPE_DOES, &last->flags
        CODE_WORD (C_EX)        //
        // Append to the word we just created a pointer to code following "_DOES,RET"
        CODE_ICP                // icp
        CODE_PUSH_NUM (2)       // icp, sizeof (addr_t)
        CODE_WORD (ADD)         // icp+sizeof(addr_t)
        CODE_WORD (_C)
        CODE_EXIT
END_WORD_CODE

//---// Named words, callable by user //---//

WORD_CODE ("(", _OB, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_PUSH_CHAR (')')
    CODE_WORD (WORD)
    CODE_WORD (DROP)
    CODE_EXIT
END_WORD_CODE

#ifdef NF_TRACE
// this implementation makes execution tracing easier (does not clutter output)
// but is larger since it's implemented in C (about 2 times larger for AVR)
// This version does not handle quoted characters in input like \n, \t etc
WORD ("WORD", WORD, WHF_TYPE_NATIVE)
    uint8_t delim = nf_pop ();
    nf_push (nf_global.dap);

    addr_t cur = nf_global.dap;
    const addr_t top = (addr_t)(NF_RAM_ADDR + NF_RAM_SIZE - 1);
    if (nf_word_eol)
        nf_word_eol = 0;
    else
        while (cur < top)
        {
            uint8_t c = nf_getc ();

            if (c == (uint8_t)'\n')
            {
                if (cur != nf_global.dap)
                    nf_word_eol = 1;
                break;
            }

            if (c == delim)
            {
                if (cur != nf_global.dap)
                    break;
                else
                    continue;
            }

            nf_writeb (cur, c);
            cur++;
        }

    nf_writeb (cur, 0);
END_WORD
#else
WORD_CODE ("WORD", WORD, WHF_TYPE_CODE)
    CODE_WORD (VHERE)           // delim, buff
    CODE_WORD (SWAP)            // buff, delim
    CODE_WORD (_N)              // buff, delim, &nf_word_eol
    CODE_RAM_VAR (nf_word_eol, 0)
    CODE_WORD (_AT)             // buff, delim, nf_word_eol
    CODE_JZ (word_nextc)        // buff, delim
        CODE_WORD (_N)          // buff, delim, &nf_word_eol
        CODE_RAM_VAR (nf_word_eol, 0)
        CODE_WORD (OFF)         // buff, delim :: nf_word_wol=FALSE
        CODE_JMP (word_end)
    CODE_LABEL (word_nextc)
        CODE_PUSH_CHAR (' ')    // buff, delim, ' '
        CODE_WORD (CHEQ)        // buff, delim, delim==' '
        CODE_JZ (word_handle_qc)
            // If separator is ' ', handle newline and tab specially
            CODE_WORD (GETC)    // buff, delim, char
            CODE_PUSH_CHAR ('\t')
            CODE_WORD (CHEQ)    // buff, delim, char, char=='\t'
            CODE_JZ (word_ch_not_tab)
            CODE_WORD (DROP)
            CODE_PUSH_CHAR (' ')// buff, delim, ' '
        CODE_LABEL (word_ch_not_tab)
            CODE_PUSH_CHAR ('\n')
            CODE_WORD (CHEQ)    // buff, delim, char, char=='\n'
            CODE_JZ (word_check_delim)
            CODE_WORD (DROP)    // buff, delim
            CODE_WORD (OVER)    // buff, delim, buff
            CODE_WORD (VHERE)   // buff, delim, buff, vhere
            CODE_WORD (NEQ)     // buff, delim, buff!==vhere
            CODE_JZ (word_end)
            CODE_WORD (_N)      // buff, delim, &nf_word_eol
            CODE_RAM_VAR (nf_word_eol, 0)
            CODE_WORD (ON)      // buff, delim :: nf_word_eol=TRUE
            CODE_JMP (word_end)
        CODE_LABEL (word_handle_qc)
            CODE_WORD (GETCQ)
    CODE_LABEL (word_check_delim)
        CODE_WORD (OVER)        // buff, delim, char, delim
        CODE_WORD (CHEQ)        // buff, delim, char, char==delim
        CODE_JZ (word_not_delim)
        CODE_WORD (DROP)        // buff, delim
        CODE_WORD (OVER)        // buff, delim, buff
        CODE_WORD (VHERE)       // buff, delim, buff, vhere
        CODE_WORD (NEQ)         // buff, delim, buff!=vhere
        CODE_JZ (word_nextc)
    CODE_LABEL (word_end)       // buff, delim
        CODE_WORD (DROP)        // buff
        CODE_PUSH_NUM (0)
        CODE_WORD (SWAP)
        CODE_WORD (C_EX)        // :: *buff='\0'
        CODE_WORD (VHERE)       // wordaddr
        CODE_EXIT
    CODE_LABEL (word_not_delim) // buff, delim, char
        CODE_WORD (ROT)         // delim, char, buff
        CODE_WORD (SWAP)        // delim, buff, char
        CODE_WORD (OVER)        // delim, buff, char, buff
        CODE_WORD (C_EX)        // delim, buff :: *buff=char
        CODE_WORD (_1_P)        // delim, buff++
        CODE_WORD (SWAP)        // buff, delim
        CODE_JMP (word_nextc)
END_WORD_CODE
#endif

#ifdef NF_GLOBAL_STATE
WORD_CODE ("STATE", STATE, WHF_TYPE_DATA)
    CODE_RAM_VAR (nf_state, 0)
END_WORD_CODE
#else
WORD ("STATE", STATE, WHF_TYPE_NATIVE)
    nf_push (NF_RAM2ADDR (&nf_ctx->state));
END_WORD
#endif

WORD_CODE ("_HDR", _HDR, WHF_TYPE_CODE)
    CODE_WORD (LAST)                    // type, str, &last
    CODE_WORD (_AT)                     // type, str, last
    CODE_WORD (HERE)                    // type, str, last, eap
    CODE_WORD (LAST)                    // type, str, last, eap, &last
    CODE_WORD (_EX)                     // type, str, last :: LAST <- eap
    CODE_WORD (_C)                      // type, str :: nf_word_t.link <- last
    CODE_WORD (SLEN)                    // type, str, len
    CODE_WORD (_1_M)                    // type, str, len-1
    CODE_WORD (ROT)                     // str, len-1, type
    CODE_WORD (OR)
    CODE_WORD (C_C)                     // str :: nf_word_t.flags <- len|WHF_TYPE_XXX
    CODE_WORD (_S_C)
    CODE_PUSH_NUM (-1)
    CODE_WORD (ALLOT)                   // remove the last zero
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("CREATE", CREATE, WHF_TYPE_CODE)
    CODE_PUSH_NUM (WHF_TYPE_DATA)
    CODE_LABEL (create_parse_word)
        CODE_PUSH_CHAR (' ')
        CODE_WORD (WORD)                // type, str
        CODE_WORD (DC_AT)               // type, str, char
        CODE_JZ (create_parse_word)
    CODE_WORD (_HDR)
    CODE_WORD (REVEAL)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("VARIABLE", VARIABLE, WHF_TYPE_CODE)
    CODE_WORD (CREATE)
    CODE_WORD (VHERE)
    CODE_WORD (_C)
    CODE_PUSH_NUM (2)
    CODE_WORD (VALLOT)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("PVARIABLE", PVARIABLE, WHF_TYPE_CODE)
    CODE_WORD (CREATE)
    CODE_PUSH_NUM (2)
    CODE_WORD (PALLOT)
    // persistent storage allocates top-down, so we'll store the
    // data pointer *after* allocation.
    CODE_WORD (PHERE)
    CODE_WORD (_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE (":", _COL, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_PUSH_NUM (WHF_TYPE_CODE)
    CODE_LABEL (col_parse_word)
        CODE_PUSH_CHAR (' ')
        CODE_WORD (WORD)                // type, str
        CODE_WORD (DC_AT)               // type, str, char
        CODE_JZ (col_parse_word)
    CODE_WORD (_HDR)
    CODE_WORD (STATE)
    CODE_WORD (ON)                      // :: STATE <- TRUE
    CODE_EXIT
END_WORD_CODE

WORD_CODE (";", _COM, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (EXIT)                    // :: add CODE_EXIT
    CODE_WORD (STATE)
    CODE_WORD (OFF)
    CODE_WORD (REVEAL)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("INTERPRET", INTERPRET, WHF_TYPE_CODE)
    CODE_LABEL (interp_all_again)
        // Parse the next word from input stream
        CODE_PUSH_CHAR (' ')
        CODE_WORD (WORD)                // str
        // Get the length of the string
        CODE_WORD (DC_AT)               // str, char
        // If empty word, this is EOL; print ok and go again
        CODE_JZ (interp_eol)
        CODE_WORD (FIND)                // word|str, flag
        CODE_WORD (_QDUP)
        // The word is in dictionary?
        CODE_JZ (not_a_word)
            // Yes, check if word is immediate
            CODE_PUSH_NUM (0)           // word, flag, 0
            CODE_WORD (GT)              // word, flag>0
            // Yes, check current interpreter state
            CODE_WORD (STATE)
            CODE_WORD (_AT)             // word, flag>0, state
            CODE_JZ (drop_execute)
                // Compile mode, check if word is immediate
                CODE_JZ (execute)       // word
                CODE_WORD (_C)          //
                CODE_JMP (interp_all_again)
            CODE_LABEL (drop_execute)
                // Interpret mode, drop word semantics flag and execute it
                CODE_WORD (DROP)        // word
            CODE_LABEL (execute)
                CODE_WORD (EXECUTE)     //
                CODE_JMP (interp_all_again)
        CODE_LABEL (not_a_word)         // str
            // No, try to interpret as a number
            CODE_WORD (DUP)             // str, str
            CODE_WORD (S2N)             // str, (n, TRUE)|(FALSE)
            CODE_JZ (interp_not_a_number)
            CODE_WORD (SWAP)            // n, str
            CODE_WORD (DROP)            // n
        CODE_LABEL (interp_number)
            // It's a number, check interpreter state
            CODE_WORD (STATE)
            CODE_WORD (_AT)
            CODE_JZ (interp_all_again)  // Interpreting state, leave number on stack
            CODE_WORD (N_C)
            CODE_JMP (interp_all_again)
        CODE_LABEL (interp_eol)
            CODE_WORD (DROP)
            // in compile state type nothing
            CODE_WORD (STATE)
            CODE_WORD (_AT)
            CODE_WORD (NOT)
            CODE_JZ (interp_all_again)
            CODE_STR ("ok\\n")
            CODE_WORD (TYPE)
            CODE_JMP (interp_all_again)
        CODE_LABEL (interp_not_a_number)
            // It's not a word or a number, try to interpret as a char
            CODE_WORD (S2C)             // (c, TRUE)|FALSE
            CODE_JZ (interp_unknown_word)
            CODE_JMP (interp_number)
        CODE_LABEL (interp_unknown_word)
            CODE_PUSH_NUM (NFE_UNKNOWN_WORD)
            CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("DOES>", DOES_G, WHF_IMMEDIATE | WHF_TYPE_CODE)
    CODE_PUSH_WORD (_DOES)      // &_DOES
    CODE_WORD (_C)              //
    CODE_PUSH_NUM (0)           // CODE_EXIT
    CODE_WORD (_C)              //
    CODE_EXIT
END_WORD_CODE

// ( str -- ( str, 0 | whdr, +1|-1 )
WORD ("FIND", FIND, WHF_TYPE_NATIVE)
    addr_t name = nf_dsp [-1];
    uint8_t name_len = nf_strlen (name);

    uint8_t i;
    for (i = nfvMax - 1; ; i--)
    {
        addr_t wp;
        for (wp = nf_ctx->voc [i]; wp; wp = nf_readw (wp))
        {
            uint8_t wf = nf_readb (wp + MEMBER_OFFSET (nf_word_t, flags));
            uint8_t wl = 1 + (wf & WHF_LENGTH_MASK);
            if (wl == name_len)
            {
                addr_t wn = wp + MEMBER_OFFSET (nf_word_t, name);
                for (wl = 0; wl < name_len; wl++)
                    if (nf_readb (name + wl) != nf_readb (wn + wl))
                        break;

                if (wl == name_len)
                {
                    nf_dsp [-1] = wp;
                    nf_push ((wf & WHF_IMMEDIATE) ? -1 : +1);
                    return;
                }
            }
        }

        if (i == 0)
            break;
    }

    nf_push (0);
END_WORD

WORD_CODE ("'", _SQ, WHF_TYPE_CODE)
        CODE_PUSH_CHAR (' ')
        CODE_WORD (WORD)                // str
        CODE_WORD (FIND)
        CODE_JZ (sq_no_word)
        CODE_EXIT
    CODE_LABEL (sq_no_word)
        CODE_PUSH_NUM (NFE_UNKNOWN_WORD)
        CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("S\\042", S_DQ, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_CHAR ('"')
    CODE_WORD (WORD)
    CODE_WORD (S_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE (".\\042", _D_DQ, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_PUSH_CHAR ('"')
    CODE_WORD (WORD)
    CODE_WORD (STATE)
    CODE_WORD (_AT)
    CODE_JZ (type_now)
        CODE_WORD (S_C)
        CODE_PUSH_WORD (TYPE)
        CODE_WORD (_C)
        CODE_EXIT
    CODE_LABEL (type_now)
        CODE_WORD (TYPE)
        CODE_EXIT
END_WORD_CODE

WORD_CODE ("IF", IF, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_WORD (HERE)                    // &jz
    CODE_PUSH_NUM (2)
    CODE_WORD (ALLOT)                   // :: reserve space for JZ (ELSE|ENDIF)
    CODE_PUSH_NUM (NFC_IF)              // &jz, IF
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("ELSE", ELSE, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_NUM (NFC_IF)              // &jz, IF, IF
    CODE_WORD (EQ)                      // &jz, IF==IF
    CODE_JZ (no_if)                     // &jz
        CODE_WORD (HERE)                // &jz, &jmp
        CODE_WORD (_2DUP)               // &jz, &jmp, &jz, &jmp
        CODE_PUSH_NUM (2)
        CODE_WORD (ALLOT)               // :: reserve space for JMP(ENDIF)
        CODE_WORD (SWAP)                // &jz, &jmp, &jmp, &jz
        CODE_WORD (SUB)                 // &jz, &jmp, &jmp-&jz
        CODE_PUSH_BIGNUM (0xc300)       // &jz, &jmp, &jmp-&jz, JZ
        CODE_WORD (ADD)                 // &jz, &jmp, JZ(&jmp-&jz)
        CODE_WORD (ROT)                 // &jmp, JZ(&jmp-&jz), &jz
        CODE_WORD (_EX)                 // &jmp :: &jz <- JZ(&jmp-&jz)
        CODE_PUSH_NUM (NFC_ELSE)        // &jmp, ELSE
        CODE_EXIT
    CODE_LABEL (no_if)
        CODE_PUSH_NUM (NFE_WRONG_NESTING)
        CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("THEN", THEN, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_WORD (DUP)                     // &jmp|&jz, ELSE|IF, ELSE|IF
    CODE_PUSH_NUM (NFC_IF)              // &jmp|&jz, ELSE|IF, ELSE|IF, IF
    CODE_WORD (EQ)                      // &jmp|&jz, ELSE|IF, ELSE|IF==IF
    CODE_JZ (not_if)                    // &jmp|&jz, ELSE|IF
        CODE_WORD (DROP)                // &jz
        CODE_PUSH_BIGNUM (0xc2fe)       // &jz, JZ
        CODE_JMP (synt_jmp)
    CODE_LABEL (not_if)
        CODE_PUSH_NUM (NFC_ELSE)
        CODE_WORD (EQ)                  // &jmp|&jz, ELSE|IF==ELSE
        CODE_JZ (not_else)              // &jmp|&jz
        CODE_PUSH_BIGNUM (0xc0fe)       // &jmp, JMP
    CODE_LABEL (synt_jmp)               // &jmp|&jz, JMP|JZ
        CODE_WORD (OVER)                // &jmp|&jz, JMP|JZ, &jmp|&jz
        CODE_WORD (HERE)                // &jmp|&jz, JMP|JZ, &jmp|&jz, here
        CODE_WORD (SWAP)                // &jmp|&jz, JMP|JZ, here, &jmp|&jz
        CODE_WORD (SUB)                 // &jmp|&jz, JMP|JZ, here-&jmp|&jz
        CODE_WORD (ADD)                 // &jmp|&jz, JMP|JZ(here-2-&jmp|&jz)
        CODE_WORD (SWAP)                // JMP|JZ(here-2-&jmp|&jz), &jmp|&jz
        CODE_WORD (_EX)                 // :: &jmp|&jz <- JMP|JZ(here-2-&jmp|&jz)
        CODE_EXIT
    CODE_LABEL (not_else)
        CODE_PUSH_NUM (NFE_WRONG_NESTING)
        CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("?DO", _QDO, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_WORD (HERE)                    // here(=&jmp)
    CODE_PUSH_NUM (2)
    CODE_WORD (ALLOT)                   // :: reserve space for JMP()
    CODE_PUSH_NUM (NFC_DO)              // &jmp, NFC_DO
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("LOOP", LOOP, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_NUM (NFC_DO)              // &jmp, NFC_DO, NFC_DO
    CODE_WORD (EQ)
    CODE_JZ (loop_wrong_nesting)        // &jmp
        CODE_WORD (HERE)                // &jmp, here(=jmptarg)
        CODE_WORD (OVER)                // &jmp, here(=jmptarg), &jmp
        CODE_WORD (SUB)                 // &jmp, jmptarg-&jmp
        CODE_PUSH_BIGNUM (0xc100)       // &jmp, jmptarg-&jmp, JMP
        CODE_WORD (ADD)                 // &jmp, jmptarg-&jmp
        CODE_WORD (OVER)                // &jmp, jmptarg-&jmp, &jmp
        CODE_WORD (_EX)                 // &jmp :: put JMP(jmptarg) in place
        CODE_PUSH_WORD (_1_P)           // &jmp, &1+
        CODE_WORD (_C)                  // &jmp
        CODE_PUSH_WORD (_DO)            // &jmp, &_DO
        CODE_WORD (_C)                  // &jmp
        CODE_WORD (HERE)                // &jmp, jmptarg
        CODE_WORD (SUB)
        CODE_PUSH_BIGNUM (0xc300)       // jmptarg-&jmp, JZ
        CODE_WORD (ADD)                 // JZ(jmptarg-&jmp)
        CODE_WORD (_C)
        CODE_EXIT
    CODE_LABEL (loop_wrong_nesting)
        CODE_PUSH_NUM (NFE_WRONG_NESTING)
        CODE_WORD (THROW)
END_WORD_CODE

WORD_CODE ("BEGIN", BEGIN, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_WORD (HERE)                    // here(=jmptarg)
    CODE_PUSH_NUM (NFC_BEGIN)           // jmptarg, NFC_BEGIN
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("UNTIL", UNTIL, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_NUM (NFC_BEGIN)           // jmptarg, NFC_BEGIN, NFC_BEGIN
    CODE_WORD (EQ)
    CODE_JZ (loop_wrong_nesting)        // jmptarg
    CODE_WORD (HERE)                    // jmptarg, here
    CODE_WORD (SUB)                     // jmptarg-here
    CODE_PUSH_BIGNUM (0xc2fe)           // jmptarg-here, JZ(-2)
    CODE_WORD (ADD)                     // JZ(jmptarg-(here+2))
    CODE_WORD (_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("WHILE", WHILE, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_NUM (NFC_BEGIN)           // jmptarg, NFC_BEGIN, NFC_BEGIN
    CODE_WORD (CHEQ)
    CODE_JZ (loop_wrong_nesting)        // jmptarg, NFC_BEGIN
    CODE_WORD (HERE)                    // jmptarg, NFC_BEGIN, &jz
    CODE_PUSH_NUM (2)
    CODE_WORD (ALLOT)
    CODE_PUSH_NUM (NFC_WHILE)           // jmptarg, NFC_BEGIN, &jz, NFC_WHILE
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("REPEAT", REPEAT, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (_CCO)
    CODE_PUSH_NUM (NFC_WHILE)           // jmptarg, NFC_BEGIN, &jz, NFC_WHILE, NFC_WHILE
    CODE_WORD (CHEQ)
    CODE_JZ (repeat_no_while)           // jmptarg, NFC_BEGIN, &jz, NFC_WHILE
    CODE_WORD (DROP)                    // jmptarg, NFC_BEGIN, &jz
    CODE_WORD (HERE)                    // jmptarg, NFC_BEGIN, &jz, here
    CODE_WORD (OVER)                    // jmptarg, NFC_BEGIN, &jz, here, &jz
    CODE_WORD (SUB)                     // jmptarg, NFC_BEGIN, &jz, here-&jz
    CODE_PUSH_BIGNUM (0xc300)           // jmptarg, NFC_BEGIN, &jz, here-&jz, JZ()
    CODE_WORD (ADD)                     // jmptarg, NFC_BEGIN, &jz, JZ(here-&jz)
    CODE_WORD (SWAP)                    // jmptarg, NFC_BEGIN, JZ(here-&jz+2), &jz
    CODE_WORD (_EX)                     // jmptarg, NFC_BEGIN :: put JZ() in place
    CODE_LABEL (repeat_no_while)
    CODE_PUSH_NUM (NFC_BEGIN)           // jmptarg, NFC_BEGIN, NFC_BEGIN
    CODE_WORD (EQ)
    CODE_JZ (loop_wrong_nesting)
    CODE_WORD (HERE)                    // jmptarg, here
    CODE_WORD (SUB)                     // jmptarg-here
    CODE_PUSH_BIGNUM (0xc0fe)           // jmptarg-here, JMP(-2)
    CODE_WORD (ADD)                     // JMP(jmptarg-(here+2))
    CODE_WORD (_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("ERROR", ERROR, WHF_TYPE_CODE)
    CODE_PUSH_CHAR ('E')
    CODE_WORD (EMIT)                    // :: print 'E'
#ifdef NF_ERROR_TEXT
    CODE_WORD (DUP)
#endif
    CODE_WORD (_D)                      // :: print error code
#ifdef NF_ERROR_TEXT
    CODE_PUSH_NUM (NFE_DOES_WRONG)
    CODE_WORD (CHEQ)
    // WARNING: if this gets too big, JMP/JZ may not reach err_print
    // If this happens, we'll need to jump to intermediate labels
    CODE_JZ (err_skip_1)
        CODE_STR ("DOES> applicable only on data words")
        CODE_JMP (err_print)
    CODE_LABEL (err_skip_1)
        CODE_PUSH_NUM (NFE_UNKNOWN_WORD)
        CODE_WORD (CHEQ)
        CODE_JZ (err_skip_2)
            CODE_STR ("Unknown word")
            CODE_JMP (err_print)
    CODE_LABEL (err_skip_2)
        CODE_PUSH_NUM (NFE_EEPROM_FULL)
        CODE_WORD (CHEQ)
        CODE_JZ (err_skip_3)
            CODE_STR ("EEPROM exhausted")
            CODE_JMP (err_print)
    CODE_LABEL (err_skip_3)
        CODE_PUSH_NUM (NFE_RAM_FULL)
        CODE_WORD (CHEQ)
        CODE_JZ (err_skip_4)
            CODE_STR ("RAM exhausted")
            CODE_JMP (err_print)
    CODE_LABEL (err_skip_4)
        CODE_PUSH_NUM (NFE_COMPILE_ONLY)
        CODE_WORD (CHEQ)
        CODE_JZ (err_skip_5)
            CODE_STR ("Can't interpret compile-only word")
            CODE_JMP (err_print)
    CODE_LABEL (err_skip_5)
        CODE_PUSH_NUM (NFE_WRONG_NESTING)
        CODE_WORD (CHEQ)
        CODE_JZ (err_skip_6)
            CODE_STR ("Wrong nesting")
            CODE_JMP (err_print)
    CODE_LABEL (err_print)
        CODE_WORD (TYPE)
    CODE_LABEL (err_skip_6)
        CODE_WORD (DROP)
#endif
    CODE_PUSH_CHAR ('\n')
    CODE_WORD (EMIT)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("EXIT", EXIT, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_PUSH_NUM (0)                   // CODE_EXIT
    CODE_WORD (_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("[", _SOB, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (STATE)
    CODE_WORD (OFF)
    CODE_EXIT
END_WORD_CODE

// Vocabulary head (must be defined before last word)
VOCABULARY (nf_voc_core_inter);

WORD_CODE ("]", _SCB, WHF_TYPE_CODE | WHF_IMMEDIATE)
    CODE_WORD (STATE)
    CODE_WORD (ON)
    CODE_EXIT
END_WORD_CODE

NF_ALIGN_EOF
