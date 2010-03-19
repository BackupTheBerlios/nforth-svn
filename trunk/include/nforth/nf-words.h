/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth interpreter macros for defining new words
*/

#ifndef __NF_WORDS_H__
#define __NF_WORDS_H__

// Join to words together into one
// Use a second call here to expand arguments, if they are expandable
#define __NF_JOIN(x, y)		x ## y
#define _NF_JOIN(x, y)		__NF_JOIN(x,y)
#define __NF_STRINGIFY(x)	#x
#define _NF_STRINGIFY(x)	__NF_STRINGIFY(x)

#define _NF_EMIT_ALIGN(n) \
    __asm (".align " _NF_STRINGIFY (n))

// WARNING: Always compile with -fno-toplevel-reorder!
#define _NF_EMIT_WORD_HDR(name, cname, flags, link) \
extern void _wh_##cname; \
__asm (_NF_FLASH_SEC "\n" \
       ".globl "_NF_STRINGIFY (_NF_ASMNAME (_wh_##cname))"\n" \
       _NF_STRINGIFY (_NF_ASMNAME (_wh_##cname))":\n" \
       "990:\t.short "_NF_STRINGIFY (link)"\n" \
       "991:\t.byte (993f-992f-1)|"_NF_STRINGIFY(flags)"\n" \
       "992:\t.ascii \""name"\"\n" \
       "993:")

#define WORD_1st(name, cname, flags, prevlink) \
    _NF_EMIT_WORD_HDR (name, cname, flags, prevlink); \
    _NF_EMIT_ALIGN (__FUNC_ALIGN); \
NF_FLASH void cname () \
{

/// Start defining a native word (e.g. implemented in machine code)
#define WORD(name, cname, flags) \
    _NF_EMIT_WORD_HDR (name, cname, flags, 991b-2-__CODE_BASE); \
    _NF_EMIT_ALIGN (__FUNC_ALIGN); \
NF_FLASH void cname () \
{

/// Finish a native word declaration
#define END_WORD \
}

#define WORD_CODE_1st(name, cname, flags, prevlink) \
    _NF_EMIT_WORD_HDR (name, cname, flags, prevlink);

/// Start the definition of a indirect-code word
#define WORD_CODE(name, cname, flags) \
    _NF_EMIT_WORD_HDR (name, cname, flags, 991b-2-__CODE_BASE);

/// Finish a indirect-code word definition
#define END_WORD_CODE

// Vocabulary head (must be defined BEFORE LAST WORD IN VOCABULARY)
#define VOCABULARY(cname) \
extern void cname; \
__asm (_NF_FLASH_SEC "\n" \
       ".globl "_NF_STRINGIFY (_NF_ASMNAME (cname))"\n" \
       _NF_STRINGIFY (_NF_ASMNAME (cname))":");

#define DECLARE_VOCABULARY(cname) \
    extern void cname;

/*
 * The following macros are used to produce indirect code
 *
 * The interpreter will pick up the indirect words and interpret them
 * according to their value as follows:
 *
 * 0000:       return from word
 * 0001:       push nf_icp from caller (the address following current word)
 * 0002..7FFF: call the word definition in flash memory at this address.
 * 8000..BFFF: call the word definition in EEPROM memory at this address - 8000.
 * C000..C1FF: (JMP) relative jump in the range -255..+255
 * C200..C3FF: (JZ) POP and relative jump if zero in the range -255..+255
 * C400..FFFF: push on stack a numeric constant in the range -7680..+7679
 */

#define _NF_EMIT_CODE(ofs) \
    __asm (".short\t" _NF_STRINGIFY (ofs));

/// This special code tells interpreter to return from this word
#define CODE_EXIT \
    _NF_EMIT_CODE (0)

/// This special instruction will push on the stack the ICP from caller
#define CODE_ICP \
    _NF_EMIT_CODE (1)

/**
 * Declare a call to a word in the flash memory
 *
 * WARNING! There is no check for the range of the offset of the word.
 * If overall dictionary size will be larger than 0x7fff, the program
 * will misbehave in an unpredictable way.
 */
#define CODE_WORD(cname) \
    _NF_EMIT_CODE (_NF_ASMNAME(_wh_##cname)-__CODE_BASE)

/**
 * Push on stack a numeric constant.
 * Small numbers will fit in a single 16-bit word.
 * Longer numbers are pushed by invoking a special word _N
 * which will fetch the 16-bit value immediately following
 * the _N opcode.
 *
 * CODE_PUSH_NUM is for numbers in the range -7680..7679
 */
#define CODE_PUSH_NUM(n) \
    _NF_EMIT_CODE (0xe200 + n)

/**
 * CODE_PUSH_BIGNUM is for large numbers in the range -32768..+32767
 */
#define CODE_PUSH_BIGNUM(n) \
    CODE_WORD (_N) \
    _NF_EMIT_CODE (n)

/// Declare a character constant
#define CODE_PUSH_CHAR(c) \
    CODE_PUSH_NUM(c)

/// Push the indirect code for a word. Works only if word address fits in CODE_PUSH_NUM
#define CODE_PUSH_WORD(cname) \
    CODE_PUSH_NUM (_NF_ASMNAME (_wh_##cname)-__CODE_BASE)

/// Push the indirect code for any word
#define CODE_PUSH_BIGWORD(cname) \
    CODE_PUSH_BIGNUM (_NF_ASMNAME (_wh_##cname)-__CODE_BASE)

/**
 * Push on the stack the address of a zero-terminated string constant.
 * This is implemented by calling a special word _S which will
 * push on the stack the address of the string immediately
 * following the address of the _S opcode.
 */
#define CODE_STR(s) \
    CODE_WORD (_S) \
    __asm (".asciz \""s"\"\n");

/// Declare a near numeric label within indirect code
#define CODE_LABEL(n) \
    __asm (".L_nfl_"#n":\n");

/// Unconditional jump to label
#define CODE_JMP(n) \
    _NF_EMIT_CODE (0xc100 + (.L_nfl_##n - 999f)) \
    __asm ("999:\n");

/// Jump if top-of-stack is zero to label; DROP
#define CODE_JZ(n) \
    _NF_EMIT_CODE (0xc300 + (.L_nfl_##n - 999f)) \
    __asm ("999:\n");

/// Place an offset to a variable in RAM: use only in WHF_TYPE_DATA words
#define CODE_RAM_VAR(var, ofs) \
    _NF_EMIT_CODE (_NF_ASMNAME (var) + ofs - __RAM_BASE + NF_RAM_ADDR)

/// Place an offset to a variable in EEPROM: use only in WHF_TYPE_DATA words
#define CODE_EEPROM_VAR(var, ofs) \
    _NF_EMIT_CODE (_NF_ASMNAME (var) + ofs - __EEPROM_BASE + NF_EEPROM_ADDR)

#endif /* __NF_WORDS_H__ */
