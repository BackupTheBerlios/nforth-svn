/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    The implementation of the MAIN word in nForth
*/

#include "nf.h"

// NEVER put C code after FORTH words because WORD_XXX macros will (mis-)use
// the .section assembler directive, and your C code may find itself where
// Makar didnt' chase his calves.

// We don't need a FORTH name for this word, so use \0 here (unnamed word)
// This doesn't prevent us from giving it the C name of MAIN
WORD_CODE_1st ("\\0", MAIN, WHF_TYPE_CODE, 0)
        CODE_STR ("\\nWelcome to nForth console!\\n");
        CODE_WORD (TYPE)

    CODE_LABEL (main_again)
        // INTERPRET never returns except if a exception occurs
        CODE_PUSH_WORD (INTERPRET)
        // Set the catch point here
        CODE_WORD (CATCH)
        CODE_WORD (ERROR)       // :: display the "Exx ..." message
        /* It would be better to reset the FORTH kernel here, but
         * this will lose any words that user defined (if he
         * didn't invoke SAVE). To avoid frustrating novices,
         * we won't do it.
         */
        //CODE_WORD (RESET)       // :: reset the interpreter
        CODE_JMP (main_again)
END_WORD_CODE

// This special word must come at the end of files with FORTH code to align
// the size of the .text section. If you see avr-gcc linking errors like:
//
//      xxx.c:1234: warning: internal error: out of range error
//
// it is likely you forgot to put this macro at the end of some source file.
// The 'out of range error' is gcc's way of telling you that some function
// lies on an unaligned address.
NF_ALIGN_EOF
