/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Interpreter core
*/

#include "nf-support.h"

WORD_1st ("GETC", GETC, WHF_TYPE_NATIVE, _NF_ASMNAME (nf_voc_core) - __CODE_BASE)
    nf_push (nf_getc ());
END_WORD

WORD_CODE ("GETCQ", GETCQ, WHF_TYPE_CODE)
    CODE_WORD (GETC)
    CODE_PUSH_CHAR ('\\')
    CODE_WORD (CHEQ)
    CODE_JZ (getcq_not_quoted)
        CODE_WORD (DROP)
        CODE_WORD (GETC)                // char
        CODE_WORD (CUNQ)
    CODE_LABEL (getcq_not_quoted)
        CODE_EXIT
END_WORD_CODE

WORD ("EMIT", EMIT, WHF_TYPE_NATIVE)
    nf_putc (nf_pop ());
END_WORD

WORD_CODE (".", _D, WHF_TYPE_CODE)
    CODE_WORD (DUP)                     // num, num
    CODE_PUSH_NUM (0)                   // num, num, 0
    CODE_WORD (LT)                      // num, num<0
    // If negative, print the sign
    CODE_JZ (not_negative)              // num
        CODE_PUSH_NUM ('-')             // :: print negative sign
        CODE_WORD (EMIT)
    CODE_LABEL (not_negative)
        CODE_PUSH_NUM (0)               // num, 0 :: end-of-digits flag
        CODE_WORD (SWAP)                // 0, num
    // Push all the digits, one by one, on the stack
    CODE_LABEL (make_digits)
        CODE_PUSH_NUM (10)              // 0, num, 10
        CODE_WORD (DIVMOD)              // 0, num%10, num/10
        CODE_WORD (SWAP)                // 0, num/10, num%10
        CODE_WORD (ABS)                 // 0, num/10, |num%10|
        CODE_PUSH_NUM ('0')             // 0, num/10, num%10, '0'
        CODE_WORD (ADD)                 // 0, num/10, num%10+'0'
        CODE_WORD (SWAP)                // 0, digit, num/10
        CODE_WORD (_QDUP)               // 0, digit, num, {? num}
        CODE_JZ (print_digits)          // 0, digit, {? num}
        CODE_JMP (make_digits)
    // Now print out all the digits and possibly the sign we have on stack
    CODE_LABEL (print_digits)           // 0, {?...}, chr
        CODE_WORD (_QDUP)               // 0, {?...}, chr, {? chr}
        CODE_JZ (finish_print)          // 0, {?...}, chr
        CODE_WORD (EMIT)                // 0, {?...} :: print next char
        CODE_JMP (print_digits)
    CODE_LABEL (finish_print)
        CODE_PUSH_NUM (' ')             // :: print a space
        CODE_WORD (EMIT)
        CODE_EXIT
END_WORD_CODE

WORD_CODE ("TYPE", TYPE, WHF_TYPE_CODE)
    CODE_LABEL (type_next_char)
        CODE_WORD (DUP)         // str, str
        CODE_WORD (_1_P)        // str, str+1
        CODE_WORD (SWAP)        // str+1, str
        CODE_WORD (C_AT)        // str+1, char
        CODE_WORD (_QDUP)       // str+1, char, char
        CODE_JZ (type_exit)
        CODE_WORD (EMIT)        // str+1
        CODE_JMP (type_next_char)
    CODE_LABEL (type_exit)
        CODE_WORD (DROP)
        CODE_EXIT
END_WORD_CODE

// Vocabulary head (must be defined before last word)
VOCABULARY (nf_voc_core_io);

WORD_CODE (".S", _DS, WHF_TYPE_CODE)
    CODE_WORD (DEPTH)
    CODE_LABEL (next_cell)
        CODE_WORD (_QDUP)
        CODE_JZ (end_print_stack)
        CODE_WORD (DUP)
        CODE_WORD (PICK)
        CODE_WORD (_D)
        CODE_PUSH_NUM (1)
        CODE_WORD (SUB)
        CODE_JMP (next_cell)
    CODE_LABEL (end_print_stack)
        CODE_EXIT
END_WORD_CODE

NF_ALIGN_EOF
