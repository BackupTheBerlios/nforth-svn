/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Interpreter core
*/

#include "nf-support.h"

#include <stdlib.h>
#include <string.h>

// Words are defined more or less in a seldom-to-frequently-used order
// (last defined words are found first during a vocabulary scan).
// This is not too important, but can save a few microseconds when
// interactively entering a program.
//
// Always compile with -fno-toplevel-reorder, otherwise gcc will
// reorder functions and you'll get a wrong vocabulary start pointer
// and word headers will go apart from function bodies.
//
// Words which name begins with a '_' are internal to nForth
// and are not advised for general use. Also there are unnamed words
// which have a name consisting of a single NULL char, which cannot
// be invoked by user directly at all.

//---// Unnamed words, never called by user directly //---//

// Push the number directly following the call to this routine
WORD_1st ("\\0", _N, WHF_TYPE_NATIVE, 0)
    nf_push (nf_readw (nf_icp));
    nf_icp += sizeof (addr_t);
END_WORD

// Push the zero-terminated string directly following the call to this routine
WORD ("\\0", _S, WHF_TYPE_NATIVE)
    nf_push (nf_icp);
    nf_icp += nf_strlen (nf_icp) + 1;
END_WORD

WORD_CODE ("\\0", _HERE, WHF_TYPE_DATA)
    CODE_RAM_VAR (nf_global, 2) // eap
END_WORD_CODE

WORD_CODE ("\\0", _VHERE, WHF_TYPE_DATA)
    CODE_RAM_VAR (nf_global, 0) // dap
END_WORD_CODE

WORD_CODE ("\\0", _PHERE, WHF_TYPE_DATA)
    CODE_RAM_VAR (nf_global, 4) // pap
END_WORD_CODE

WORD_CODE ("\\0", _DO, WHF_TYPE_CODE)
    CODE_WORD (_2DUP)
    CODE_WORD (LEQ)
    CODE_WORD (DUP)
    CODE_JZ (_do_loop)
        CODE_WORD (DROP)
        CODE_WORD (DROP)
        CODE_WORD (DROP)
        CODE_WORD (TRUE)
    CODE_LABEL (_do_loop)
        CODE_EXIT
END_WORD_CODE

// Compile a string
WORD_CODE ("\\0", _S_C, WHF_TYPE_CODE)
    CODE_LABEL (strcomp_copy_name)
        CODE_WORD (DUP)                 // str, str
        CODE_WORD (_1_P)                // str, str+1
        CODE_WORD (SWAP)                // str+1, str
        CODE_WORD (C_AT)                // str+1, char
        CODE_WORD (DUP)                 // str+1, char, char
        CODE_WORD (C_C)                 // str+1, char :: nf_word_t.name[i] <- c
        CODE_PUSH_NUM (0)
        CODE_WORD (EQ)                  // str+1, char==0
        CODE_JZ (strcomp_copy_name)     // str+1
    CODE_WORD (DROP)
    CODE_EXIT
END_WORD_CODE

// Convert a string to a positive number
WORD_CODE ("\\0", _S2N, WHF_TYPE_CODE)
        CODE_WORD (DC_AT)       // str, char
        CODE_PUSH_CHAR ('0')    // str, char, '0'
        CODE_WORD (EQ)          // str, char=='0'
        CODE_JZ (s2n_dec)
        CODE_WORD (_1_P)
        CODE_WORD (DC_AT)       // str, char
        CODE_PUSH_CHAR ('x')    // str, char, 'x'
        CODE_WORD (EQ)          // str, char=='x'
        CODE_JZ (s2n_dec)
        CODE_WORD (_1_P)        // str+1
        CODE_PUSH_NUM (16)      // str, 16
        CODE_JMP (s2n_set_radix)
    CODE_LABEL (s2n_dec)
        CODE_PUSH_NUM (10)      // str, 10
    CODE_LABEL (s2n_set_radix)
        CODE_PUSH_NUM (0)       // str, base, val
        CODE_WORD (ROT)         // base, val, str
    CODE_LABEL (s2n_next_digit)
        CODE_WORD (DC_AT)       // base, val, str, char
        CODE_WORD (_QDUP)
        CODE_JZ (s2n_num_done)
        CODE_PUSH_CHAR ('0')
        CODE_WORD (SUB)         // base, val, str, char-'0'(=digit)
        CODE_WORD (DUP)
        CODE_PUSH_NUM (0)
        CODE_WORD (GEQ)         // base, val, str, digit, digit>=0
        CODE_JZ (s2n_badnum)
        CODE_WORD (DUP)
        CODE_PUSH_NUM (9)
        CODE_WORD (GT)          // base, val, str, digit, digit>9
        CODE_JZ (s2n_digit_ok)
        CODE_PUSH_NUM (7)       // :: subtract 'A' - '9' + 1
        CODE_WORD (SUB)
    CODE_LABEL (s2n_digit_ok)   // base, val, str, digit
        CODE_WORD (DUP)         // base, val, str, digit, digit
        CODE_PUSH_NUM (4)
        CODE_WORD (PICK)        // base, val, str, digit, digit, base
        CODE_WORD (LT)          // base, val, str, digit, digit<base
        CODE_JZ (s2n_badnum)
        CODE_WORD (ROT)         // base, str, digit, val
        CODE_PUSH_NUM (3)
        CODE_WORD (PICK)        // base, str, digit, val, base
        CODE_WORD (MUL)         // base, str, digit, val*base
        CODE_WORD (ADD)         // base, str, digit+val*base
        CODE_WORD (SWAP)        // base, val, str
        CODE_WORD (_1_P)        // base, val, str+1
        CODE_JMP (s2n_next_digit)
    CODE_LABEL (s2n_badnum)
        CODE_WORD (DROP)        // base, val, str
        CODE_WORD (DROP)        // base, val
        CODE_WORD (DROP)        // base
        CODE_WORD (DROP)        //
        CODE_WORD (FALSE)       // FALSE
        CODE_EXIT
    CODE_LABEL (s2n_num_done)
        CODE_WORD (DROP)        // base, val
        CODE_WORD (SWAP)        // val, base
        CODE_WORD (DROP)        // val
        CODE_WORD (TRUE)        // val, TRUE
        CODE_EXIT
END_WORD_CODE

// Translate n -> \n, r -> \r, t -> \t etc
// ( char -- char )
WORD_CODE ("\\0", CUNQ, WHF_TYPE_CODE)
    CODE_STR ("n\\nr\\rt\\tb\\b")       // char, delim
    CODE_LABEL (cunq_nextc)
        CODE_WORD (DC_AT)               // char, delim, dch
        CODE_WORD (_QDUP)               // char, delim, dch, {?dch}
        CODE_JZ (cunq_not_in_list)
        CODE_PUSH_NUM (2)
        CODE_WORD (PICK)                // char, delim, dch, char
        CODE_WORD (EQ)                  // char, delim, dch==char
        CODE_JZ (cunq_not_dch)
            CODE_WORD (_1_P)            // char, delim+1
            CODE_WORD (C_AT)            // char, replacement
            CODE_WORD (SWAP)
            CODE_JMP (cunq_not_in_list)
        CODE_LABEL (cunq_not_dch)
            CODE_PUSH_NUM (2)
            CODE_WORD (ADD)            // char, delim+2
            CODE_JMP (cunq_nextc)
    CODE_LABEL (cunq_not_in_list)
        CODE_WORD (DROP)
        CODE_EXIT
END_WORD_CODE

//---// Named words, callable by user //---//

WORD ("RESET", RESET, WHF_TYPE_NATIVE)
    // Reset machine state using EEPROM stored values
    nf_memcpy (NF_RAM2ADDR (&nf_global.dap), NF_EEPROM2ADDR (&nf_global_save.dap), sizeof (nf_global));
#ifdef NF_GLOBAL_STATE
    nf_state = 0;
#endif
    nf_app_reset ();
END_WORD

WORD ("SAVE", SAVE, WHF_TYPE_NATIVE)
    nf_memcpy (NF_EEPROM2ADDR (&nf_global_save.dap), NF_RAM2ADDR (&nf_global.dap), sizeof (nf_global));
END_WORD

WORD ("CLEAN", CLEAN, WHF_TYPE_NATIVE)
    nf_dsp = (cell_t *)&nf_global;
    nf_push (NF_RAM2ADDR (&__RAM_END));        // dap
    nf_push (NF_EEPROM2ADDR (&__EEPROM_END));   // eap
    nf_push (NF_EEPROM_ADDR + NF_EEPROM_SIZE);  // pap
    nf_push (0);                                // last

    nf_app_clean ();

    SAVE ();
    RESET ();
END_WORD

// This is the core of the nForth interpreter
// It takes the address of a word on the stack and executes it.
// ( word -- )
WORD ("EXECUTE", EXECUTE, WHF_TYPE_NATIVE)
    // pick up the address of the word to execute
    addr_t tmp = nf_pop ();

    NF_TRACE_WORD ("EXECUTE", tmp);

    // Is this a special value rather than an word address?
    if (tmp >= 0xc000)
    {
        if (tmp < 0xc200)               // JUMP
            nf_icp += (tmp - 0xc100);
        else if (tmp < 0xc400)          // JZ
        {
            if (nf_pop () == 0)
                nf_icp += (tmp - 0xc300);
        }
        else                            // NUM
            nf_push (tmp - 0xe200);
        return;
    }

    // pick word flags
    uint8_t flags = nf_readb (tmp + MEMBER_OFFSET (nf_word_t, flags));
    // compute the address of word body
    tmp += MEMBER_OFFSET (nf_word_t, name) + 1 + (flags & WHF_LENGTH_MASK);
    switch ((uint8_t)(flags & WHF_TYPE_MASK))
    {
        case WHF_TYPE_NATIVE:
            // Call the function at virtual address tmp
            nf_call (NF_ADDR2CODE (tmp));
            break;
        case WHF_TYPE_DATA:
            // Push the address of word data onto the stack
            nf_push (nf_readw (tmp));
            break;
        case WHF_TYPE_DOES:
            // Push the address of word data onto the stack
            nf_push (nf_readw (tmp));
            // Get the address of the code following DOES>
            tmp = nf_readw (tmp + 2);
            // fallback to WHF_TYPE_CODE
        case WHF_TYPE_CODE:
        {
            // tmp holds the outer nf_icp
            nf_xchg (tmp, nf_icp);
            for (;;)
            {
                // Read next indirect code
                addr_t x = nf_readw (nf_icp);
                // Advance to next code
                nf_icp += sizeof (addr_t);
                // Check for special opcodes
                if (!(uint8_t)(x >> 8))
                {
                    if (((uint8_t)x) == 0)
                    {
                        // CODE_EXIT - return from indirect code word
                        break;
                    }
                    else if (((uint8_t)x) == 1)
                    {
                        // CODE_ICP - push the outer icp
                        nf_push (tmp);
                        continue;
                    }
                }

                // Push the address of word to execute
                nf_push (x);
                // Execute it (recursively call ourselves)
                NF_TRACE_LEVEL_INC;
                EXECUTE ();
                NF_TRACE_LEVEL_DEC;
            }
            nf_icp = tmp;
            break;
        }
    }
END_WORD

WORD ("THROW", THROW, WHF_TYPE_NATIVE)
    nf_throw (nf_pop ());
END_WORD

WORD ("CATCH", CATCH, WHF_TYPE_NATIVE)
    nf_catch ();
END_WORD

// Can't use RAMEND here as C name because it is a macro
WORD_CODE ("RAMEND", _RAMEND, WHF_TYPE_CODE)
#if NF_RAM_SIZE < 7680
    CODE_PUSH_NUM (NF_RAM_SIZE);
#else
    CODE_PUSH_BIGNUM (NF_RAM_SIZE);
#endif
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("E2END", _E2END, WHF_TYPE_CODE)
#if NF_EEPROM_SIZE < 7680
    CODE_PUSH_NUM (NF_EEPROM_SIZE);
#else
    CODE_PUSH_BIGNUM (NF_EEPROM_SIZE);
#endif
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("LAST", LAST, WHF_TYPE_DATA)
    CODE_RAM_VAR (nf_global, 6) // last
END_WORD_CODE

WORD_CODE ("HERE", HERE, WHF_TYPE_CODE)
    CODE_WORD (_HERE)
    CODE_WORD (_AT)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("VHERE", VHERE, WHF_TYPE_CODE)
    CODE_WORD (_VHERE)
    CODE_WORD (_AT)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("PHERE", PHERE, WHF_TYPE_CODE)
    CODE_WORD (_PHERE)
    CODE_WORD (_AT)
    CODE_EXIT
END_WORD_CODE

WORD ("REVEAL", REVEAL, WHF_TYPE_NATIVE)
    // Add the last defined word to user dictionary
    nf_ctx->voc [nfvUser] = nf_global.last;
END_WORD

// This word allocates vocabulary space in EEPROM
// by incrementing an allocation pointer
WORD_CODE ("ALLOT", ALLOT, WHF_TYPE_CODE)
    CODE_WORD (HERE)            // size, here
    CODE_WORD (ADD)             // here+size
    CODE_WORD (DUP)             // here+size, here+size
    CODE_WORD (PHERE)           // here+size, here+size, phere
    CODE_WORD (GEQ)             // here+size, here+size>=phere
    CODE_JZ (allot_ok)
        CODE_PUSH_NUM (NFE_EEPROM_FULL)
        CODE_WORD (THROW)
    CODE_LABEL (allot_ok)
        CODE_WORD (_HERE)       // here+size, &here
        CODE_WORD (_EX)         // :: here = here+size
        CODE_EXIT
END_WORD_CODE

// This word allocates variable space in RAM
// by incrementing an allocation pointer
WORD_CODE ("VALLOT", VALLOT, WHF_TYPE_CODE)
    CODE_WORD (VHERE)           // size, vhere
    CODE_WORD (ADD)             // vhere+size
    CODE_WORD (DUP)             // vhere+size, vhere+size
    CODE_WORD (_RAMEND)         // vhere+size, vhere+size, ramend
    CODE_WORD (GEQ)             // vhere+size, vhere+size>=ramend
    CODE_JZ (vallot_ok)
        CODE_PUSH_NUM (NFE_RAM_FULL)
        CODE_WORD (THROW)
    CODE_LABEL (vallot_ok)
        CODE_WORD (_VHERE)      // vhere+size, &vhere
        CODE_WORD (_EX)         // :: vhere = vhere+size
        CODE_EXIT
END_WORD_CODE

// This word allocates persistent variable space in EEPROM
// by decrementing an allocation pointer
WORD_CODE ("PALLOT", PALLOT, WHF_TYPE_CODE)
    CODE_WORD (PHERE)           // size, phere
    CODE_WORD (SWAP)            // phere, size
    CODE_WORD (SUB)             // phere-size
    CODE_WORD (DUP)             // phere-size, phere-size
    CODE_WORD (HERE)            // phere-size, phere-size, here
    CODE_WORD (LEQ)             // phere-size, phere-size<=here
    CODE_JZ (pallot_ok)
        CODE_PUSH_NUM (NFE_EEPROM_FULL)
        CODE_WORD (THROW)
    CODE_LABEL (pallot_ok)
        CODE_WORD (_PHERE)      // phere-size, &phere
        CODE_WORD (_EX)         // :: phere = phere-size
        CODE_EXIT
END_WORD_CODE

WORD_CODE (",", _C, WHF_TYPE_CODE)
    CODE_WORD (HERE)
    CODE_PUSH_NUM (2)
    CODE_WORD (ALLOT)
    CODE_WORD (_EX)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("C,", C_C, WHF_TYPE_CODE)
    CODE_WORD (HERE)
    CODE_PUSH_NUM (1)
    CODE_WORD (ALLOT)
    CODE_WORD (C_EX)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("S,", S_C, WHF_TYPE_CODE)
    CODE_PUSH_WORD (_S)         // str, &_S
    CODE_WORD (_C)              // str :: compile _S
    CODE_WORD (_S_C)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("N,", N_C, WHF_TYPE_CODE)
    // Compile the number (emulate either CODE_PUSH_NUM or CODE_PUSH_BIGNUM)
    CODE_WORD (DUP)             // n, n
    CODE_PUSH_NUM (-7680)       // n, n, -7680
    CODE_WORD (GEQ)             // n, n>=-7680
    CODE_JZ (n_c_big_number)    // n
    CODE_WORD (DUP)             // n, n
    CODE_PUSH_NUM (7679)        // n, n, 7679
    CODE_WORD (LEQ)             // n, n<=7679
    CODE_JZ (n_c_big_number)    // n
    // A number in range -7680..+7679 is coded in a single word
    CODE_PUSH_NUM (-7680)       // n, -7680
    CODE_WORD (ADD)             // n-7680
    CODE_WORD (_C)              //
    CODE_EXIT
    CODE_LABEL (n_c_big_number)
    CODE_PUSH_WORD (_N)         // n, &_N
    CODE_WORD (_C)              // n :: compile ref to _N
    CODE_WORD (_C)              // :: compile the big number
    CODE_EXIT
END_WORD_CODE

WORD ("SLEN", SLEN, WHF_TYPE_NATIVE)
    addr_t str = nf_dsp [-1];
    nf_push (nf_strlen (str));
END_WORD

// ( str -- [ n TRUE | FALSE ] )
WORD_CODE ("S2N", S2N, WHF_TYPE_CODE)
    CODE_WORD (DC_AT)           // str, char
    CODE_PUSH_CHAR ('-')        // str, char, '-'
    CODE_WORD (CHEQ)            // str, char, char=='-'
    CODE_JZ (s2n_positive)
        CODE_WORD (DROP)
        CODE_WORD (_1_P)        // str+1(=str)
        CODE_WORD (_S2N)
        CODE_WORD (SWAP)
        CODE_WORD (NEGATE)
        CODE_WORD (SWAP)
        CODE_EXIT
    CODE_LABEL (s2n_positive)
        CODE_PUSH_CHAR ('+')    // str, char, '+'
        CODE_WORD (EQ)          // str, char=='+'
        CODE_JZ (s2n_not_plus)
        CODE_WORD (_1_P)        // :: skip positive sign
    CODE_LABEL (s2n_not_plus)
        CODE_WORD (_S2N)
        CODE_EXIT
END_WORD_CODE

// ( str -- [ n TRUE | FALSE ] )
WORD_CODE ("S2C", S2C, WHF_TYPE_CODE)
    CODE_WORD (DC_AT)           // str, char
    CODE_PUSH_CHAR ('\'')
    CODE_WORD (EQ)              // str, char=='\''
    CODE_JZ (s2c_error)
        CODE_WORD (_1_P)        // str+1(=str)
        CODE_WORD (DC_AT)       // str, char
        CODE_PUSH_CHAR ('\\')
        CODE_WORD (CHEQ)        // str, char, char=='\\'
        CODE_JZ (s2c_not_backsl)
        CODE_WORD (DROP)        // str
        CODE_WORD (_1_P)        // str+1(=str)
        CODE_WORD (DC_AT)       // str, char
        CODE_WORD (CUNQ)        // str, char
    CODE_LABEL (s2c_not_backsl)
        CODE_WORD (_QDUP)       // str, char{, char}
        CODE_JZ (s2c_error)     // str{, char}
        CODE_WORD (SWAP)        // char, str
        CODE_WORD (_1_P)        // char, str+1(=str)
        CODE_WORD (C_AT)        // char, char
        CODE_PUSH_CHAR ('\'')
        CODE_WORD (EQ)          // char, char=='\''
        CODE_JZ (s2c_error)
        CODE_WORD (TRUE)
        CODE_EXIT
    CODE_LABEL (s2c_error)
        CODE_WORD (DROP)
        CODE_WORD (FALSE)
        CODE_EXIT
END_WORD_CODE

WORD ("@", _AT, WHF_TYPE_NATIVE)
    cell_t addr = nf_pop ();
    nf_push (nf_readw (addr));
END_WORD

WORD ("!", _EX, WHF_TYPE_NATIVE)
    cell_t addr = nf_pop ();
    cell_t val = nf_pop ();
    nf_writew (addr, val);
END_WORD

WORD ("C@", C_AT, WHF_TYPE_NATIVE)
    cell_t addr = nf_pop ();
    nf_push (nf_readb (addr));
END_WORD

WORD_CODE ("DC@", DC_AT, WHF_TYPE_CODE)
    CODE_WORD (DUP)
    CODE_WORD (C_AT)
    CODE_EXIT
END_WORD_CODE

WORD ("C!", C_EX, WHF_TYPE_NATIVE)
    cell_t addr = nf_pop ();
    cell_t val = nf_pop ();
    nf_writeb (addr, val);
END_WORD

WORD_CODE ("CHEQ", CHEQ, WHF_TYPE_CODE)
    CODE_WORD (OVER)
    CODE_WORD (EQ)
    CODE_EXIT
END_WORD_CODE

WORD ("MIN", MIN, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 < arg2) ? arg1 : arg2);
END_WORD

WORD ("MAX", MAX, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 > arg2 ? arg1 : arg2);
END_WORD

WORD_CODE ("TRUE", TRUE, WHF_TYPE_CODE)
    CODE_PUSH_NUM (-1)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("FALSE", FALSE, WHF_TYPE_CODE)
    CODE_PUSH_NUM (0)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("ON", ON, WHF_TYPE_CODE)
    CODE_PUSH_NUM (-1)
    CODE_WORD (SWAP)
    CODE_WORD (_EX)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("OFF", OFF, WHF_TYPE_CODE)
    CODE_PUSH_NUM (0)
    CODE_WORD (SWAP)
    CODE_WORD (_EX)
    CODE_EXIT
END_WORD_CODE

WORD ("DEPTH", DEPTH, WHF_TYPE_NATIVE)
    cell_t tmp = nf_dsp - nf_ctx->stack;
    nf_push (tmp);
END_WORD

WORD ("AND", AND, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 & arg2);
END_WORD

WORD ("OR", OR, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 | arg2);
END_WORD

WORD ("XOR", XOR, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 ^ arg2);
END_WORD

WORD ("NOT", NOT, WHF_TYPE_NATIVE)
    cell_t arg = nf_pop ();
    nf_push (!arg);
END_WORD

WORD ("INVERT", INVERT, WHF_TYPE_NATIVE)
    cell_t arg = nf_pop ();
    nf_push (~arg);
END_WORD

WORD ("NEGATE", NEGATE, WHF_TYPE_NATIVE)
    cell_t arg = nf_pop ();
    nf_push (-arg);
END_WORD

// This does not work correctly for -32768
WORD_CODE ("ABS", ABS, WHF_TYPE_CODE)
    CODE_WORD (DUP)
    CODE_PUSH_NUM (0)
    CODE_WORD (LT)
    CODE_JZ (abs_not_negative)
        CODE_WORD (NEGATE)
    CODE_LABEL (abs_not_negative)
        CODE_EXIT
END_WORD_CODE

WORD ("<>", NEQ, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 != arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD ("=", EQ, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 == arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD ("<=", LEQ, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 <= arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD (">=", GEQ, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 >= arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD ("<", LT, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 < arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD (">", GT, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push ((arg1 > arg2) ? NF_TRUE : NF_FALSE);
END_WORD

WORD_CODE ("1+", _1_P, WHF_TYPE_CODE)
    CODE_PUSH_NUM (1)
    CODE_WORD (ADD)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("1-", _1_M, WHF_TYPE_CODE)
    CODE_PUSH_NUM (1)
    CODE_WORD (SUB)
    CODE_EXIT
END_WORD_CODE

WORD ("/MOD", DIVMOD, WHF_TYPE_NATIVE)
    cell_t denom = nf_pop ();
    cell_t num = nf_pop ();
    div_t r = div (num, denom);
    nf_push (r.rem);
    nf_push (r.quot);
END_WORD

WORD_CODE ("/", DIV, WHF_TYPE_CODE)
    CODE_WORD (DIVMOD)
    CODE_WORD (SWAP)
    CODE_WORD (DROP)
    CODE_EXIT
END_WORD_CODE

WORD_CODE ("MOD", MOD, WHF_TYPE_CODE)
    CODE_WORD (DIVMOD)
    CODE_WORD (DROP)
    CODE_EXIT
END_WORD_CODE

WORD ("*", MUL, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 * arg2);
END_WORD

WORD ("-", SUB, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 - arg2);
END_WORD

WORD ("+", ADD, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg1 + arg2);
END_WORD

WORD ("SWAP", SWAP, WHF_TYPE_NATIVE)
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg2);
    nf_push (arg1);
END_WORD

WORD ("OVER", OVER, WHF_TYPE_NATIVE)
    cell_t tmp = nf_dsp [-2];
    nf_push (tmp);
END_WORD

WORD ("ROT", ROT, WHF_TYPE_NATIVE)
    cell_t arg3 = nf_pop ();
    cell_t arg2 = nf_pop ();
    cell_t arg1 = nf_pop ();
    nf_push (arg2);
    nf_push (arg3);
    nf_push (arg1);
END_WORD

WORD ("PICK", PICK, WHF_TYPE_NATIVE)
    cell_t tmp = nf_pop ();
    tmp = nf_dsp [-1 - tmp];
    nf_push (tmp);
END_WORD

WORD_CODE ("2DUP", _2DUP, WHF_TYPE_CODE)
    CODE_WORD (OVER)
    CODE_WORD (OVER)
    CODE_EXIT
END_WORD_CODE

WORD ("DUP", DUP, WHF_TYPE_NATIVE)
    cell_t tmp = nf_dsp [-1];
    nf_push (tmp);
END_WORD

WORD ("?DUP", _QDUP, WHF_TYPE_NATIVE)
    cell_t tmp = nf_dsp [-1];
    if (tmp)
        nf_push (tmp);
END_WORD

WORD_CODE ("+!", _ADD_EX, WHF_TYPE_CODE)
    CODE_WORD (DUP)             // n, addr, addr
    CODE_WORD (_AT)             // n, addr, val
    CODE_WORD (ROT)             // addr, val, n
    CODE_WORD (ADD)             // addr, val+n
    CODE_WORD (SWAP)            // val+n, addr
    CODE_WORD (_EX)
    CODE_EXIT
END_WORD_CODE

// Vocabulary head (must be defined before last word)
VOCABULARY (nf_voc_core);

WORD ("DROP", DROP, WHF_TYPE_NATIVE)
    nf_dsp--;
END_WORD

NF_ALIGN_EOF
