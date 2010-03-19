/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth interpreter public client API
*/

#ifndef __NF_H__
#define __NF_H__

#include <stdint.h>

#include "nf-defs.h"
#include "nf-words.h"

/**
 * To make the life easier, we define a uniform memory space
 * for accessing RAM, ROM and EEPROM spaces (which are separate
 * on Harvard architecture) within a common 16-bit address space.
 * For this, we split the 16-bit address space into three segments:
 *
 * 0000..7FFF: flash memory
 * 8000..7FFF+EEPROM_SIZE: EEPROM
 * 8000+EEPROM_SIZE..7FFF+EEPROM_SIZE+RAM_SIZE: RAM
 *
 * For more details, see the "memory organization" section in
 * nForth documentation.
 */
#define NF_FLASH_ADDR		0x0000
#define NF_EEPROM_ADDR		0x8000
#define NF_RAM_ADDR		(NF_EEPROM_ADDR + NF_EEPROM_SIZE)

/**
 * Data stack cell type - signed 16-bit.
 * Range: -32768...+32767
 */
typedef int16_t cell_t;

/**
 * The "virtual address" type, represents an address in the 
 * NOTE: sizeof (addr_t) must be == sizeof (cell_t)
 */
typedef uint16_t addr_t;

/// Boolean true
#define NF_TRUE			((cell_t)-1)
/// Boolean false
#define NF_FALSE		((cell_t)0)

/** The word header structure in vocabulary */
typedef struct
{
    /// Pointer to previous word in vocabulary
    addr_t link;
    /// Word flags (see WHF_XXX constants)
    uint8_t flags;
    /// Followed by variable-length word name
    uint8_t name [0];
    /// Followed either by code, indirect code or data
} __attribute__((packed)) nf_word_t;

/// The word is immediate
#define WHF_IMMEDIATE   	0x80
/// Word type mask
#define WHF_TYPE_MASK		0x60
/// Word contains a pointer to data
#define WHF_TYPE_DATA		0x00
/// Word contains indirect threaded code
#define WHF_TYPE_CODE		0x20
/// Word contains native code
#define WHF_TYPE_NATIVE		0x40
/// This is a DOES> word - name followed by a data pointer, then a code pointer
#define WHF_TYPE_DOES		0x60
/// Name length mask - length is actually by 1 longer (no zero-length names)
#define WHF_LENGTH_MASK 	0x1f

/**
 * nForth vocabulary indices within nf_ctx->voc[].
 */
enum
{
    nfvCore,
    nfvUser,
    nfvMax
};

/// Core vocabulary (no interactive stuff)
DECLARE_VOCABULARY (nf_voc_core);
/// Core vocabulary + terminal I/O (EMIT, TYPE etc)
DECLARE_VOCABULARY (nf_voc_core_io);
/// Core vocabulary + terminal I/O + interactive words (INTERPRET, FIND etc + immediate words)
DECLARE_VOCABULARY (nf_voc_core_inter);

/**
 * This structure defines a nForth context.
 *
 * You can create an arbitrary number of such structures to implement
 * various flavours of multi-tasking.
 *
 * A global pointer to current context is held globally in the nf_ctx
 * variable.
 */
typedef struct
{
    /// Active vocabularies (zero-terminated)
    cell_t voc [nfvMax];
#ifndef NF_GLOBAL_STATE
    cell_t state;
#endif
#ifdef NF_MULTITASK
    /// Saved data stack pointer (grows from 0 upwards)
    void *save_dsp;
    /// Return stack pointer (grows from NF_STACK_SIZE downwards)
    void *save_rsp;
#endif
    /// The pointer to current save buffer for CATCH/THROW
    void *catch_ptr;
    /// The data stack (size depends on available RAM)
    cell_t stack [NF_STACK_SIZE];
} nf_context_t;

/// Data stack pointer for current context (address of the first unused byte on stack)
register cell_t *nf_dsp __asm (__R_DSP);
/// Indirect Code Pointer
register addr_t nf_icp __asm (__R_ICP);

/// Read a byte from uniform memory space
extern uint8_t nf_readb (addr_t addr);
/// Read a word from uniform memory space
extern uint16_t nf_readw (addr_t addr);
/// Write a byte to uniform memory space
extern void nf_writeb (addr_t addr, uint8_t val);
/// Write a word to uniform memory space
extern void nf_writew (addr_t addr, uint16_t val);

/**
 * Perform initialization of a nForth context.
 * @arg ctx
 *     The nForth context to initialize.
 */
extern void nf_reset_ctx (nf_context_t *ctx);

/**
 * This function is called from RESET and should initialize
 * the I/O device.
 */
extern void nf_init_io ();

/**
 * Read a single character from whatever I/O device we have
 * (terminal, serial etc).
 */
extern uint8_t nf_getc ();

/**
 * Write a single character to whatever I/O device we have
 */
extern void nf_putc (uint8_t c);

/**
 * A complete cleanup of the application.
 * The whole EEPROM state is reset. Application is expected
 * to set own EEPROM variables to their default values.
 */
extern void nf_app_clean ();

/**
 * A global reset function.
 * This function is expected to reset the state of all RAM variables
 * including nForth contexts.
 */
extern void nf_app_reset ();

/// No error
#define NFE_OK			0
/// DOES> is applicable only on data words
#define NFE_DOES_WRONG		1
/// Unknown word encountered in input
#define NFE_UNKNOWN_WORD	2
/// EEPROM memory exhausted
#define NFE_EEPROM_FULL		3
/// RAM memory exhausted
#define NFE_RAM_FULL		4
/// Compile-only word invoked in interpret mode
#define NFE_COMPILE_ONLY        5
/// Wrong control structures nesting
#define NFE_WRONG_NESTING       6

/// Identifies the IF control statement (arbitrary number)
#define NFC_IF                  233
/// Identifies the ELSE statement
#define NFC_ELSE                234
/// Identifies the DO statement
#define NFC_DO                  235
/// Latest statement is a BEGIN
#define NFC_BEGIN               236
/// Latest statement is WHILE
#define NFC_WHILE               237

/// Current nForth context is always kept in a global variable
extern NF_RAM nf_context_t *nf_ctx;
#ifdef NF_GLOBAL_STATE
/// The storage for the STATE variable
extern NF_RAM cell_t nf_state;
#endif

/**
 * This structure holds the global nForth machine state.
 * A copy of this structure is saved in EEPROM with the SAVE
 * command. At bootup this structure is read back from ROM,
 * so nForth machine restores its last state except the
 * contents of the RAM (which are supposed to be initialized
 * in nf_app_reset()).
 *
 * If you change this structure, don't forget to fix the words
 * CLEAN, _HERE, _VHERE, _PHERE, LAST which use direct offsets
 * within the structure to access fields.
 */
typedef struct
{
    /// Data Allocation Pointer (first unallocated address in RAM)
    addr_t dap;
    /// EEPROM Allocation Pointer (lowest unallocated address in EEPROM)
    addr_t eap;
    /// Persistent Variables allocation pointer (grows top-down, starts at EEPROM top)
    addr_t pap;
    /// The address of last defined word (LAST)
    addr_t last;
} __attribute__((packed)) nf_global_state_t;

/// The active nForth machine state
extern NF_RAM nf_global_state_t nf_global;
/// The saved nForth machine state
extern NF_EEPROM nf_global_state_t nf_global_save;

#ifndef nf_push
/// Push a number on the stack
#define nf_push(n)	*nf_dsp++ = (n)
#endif
#ifndef nf_pop
/// Pop a number off the stack
#define nf_pop()	(*--nf_dsp)
#endif

/**
 * This macro will execute some nForth word.
 * You may push any arguments it needs on the stack
 * before invoking it using nf_push(), but make sure
 * you first initialize the nForth context.
 */
#define nf_execute(word) \
    { \
        extern void _wh_##word; \
        extern void EXECUTE (); \
        nf_push (NF_CODE2ADDR (&_wh_##word)); \
        EXECUTE (); \
    }

/**
 * This is actually the RESET FORTH word, which will clean the RAM but will
 * preserve the EEPROM (and assumes EEPROM has been initialized sometimes
 * with valid values). You can call it in main() of your program if you're
 * sure that EEPROM content is valid.
 */
extern void RESET ();

/**
 * This is actually the CLEAN FORTH word, which will clean RAM and EEPROM,
 * will re-fill EEPROM with vital data and in the end will call RESET
 * internally so that your nForth machine is ready to be started.
 */
extern void CLEAN ();

#endif /* __NF_H__ */
