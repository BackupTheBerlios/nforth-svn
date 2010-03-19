/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth interpreter: interpreter configuration
*/

#ifndef __NF_DEFS_H__
#define __NF_DEFS_H__

/*
 * On architectures with pointers larger than 16 bits we have to
 * subtract a certain value from every text and data offset to
 * fit in 16 bits. This value is used then in linker script file
 * as the base address for the respective section.
 *
 * On platforms with 16-bit address space just define this to 0.
 */
#if defined ARCH_AVR
# define __CODE_BASE	0
# define __EEPROM_BASE	0
# define __RAM_BASE	0

# define __CODE_END	_etext
# define __EEPROM_END	__eeprom_end
# define __RAM_END	__heap_start

extern void _etext, __eeprom_end, __heap_start;

#elif defined ARCH_X86
# define __CODE_BASE	0x0000
# define __EEPROM_BASE	0x8000
# define __RAM_BASE	0xc000

#elif defined ARCH_X86_64
# define __CODE_BASE	0x610000
# define __EEPROM_BASE	0x620000
# define __RAM_BASE	0x630000

#endif

/**
 * Data (+return in multi-task configs) stack size on this platform (in bytes).
 * Data stack grows upwards while return stack grows downwards, thus the
 * memory is used effectively (unless stacks meet somewhere, which whill
 * lead to an almost guaranteed crash, heh).
 */
#ifdef ARCH_AVR
#define NF_STACK_SIZE	256
#else
#define NF_STACK_SIZE	4096
#endif

/*
 * Define this if you want a multi-tasked nForth interpreter.
 * If multi-tasking is enabled, the stack is shared between data and return stacks.
 * The top of the data stack will be used as the return stack. The context
 * switching routine is responsible for re/storing the stack pointer at task
 * switching time.
 */
//#define NF_MULTITASK

/**
 * Define this for a single global STATE variable for all contexts
 * (saves RAM if you don't use INTERPRET in more than one context).
 *
 * If you're using one context for interaction with user, and other
 * context just for executing nForth words (which don't depend on
 * the interpreter STATE), you can define this to save one cell
 * per context.
 */
#define NF_GLOBAL_STATE

/**
 * Define the following macro to get full exception error text.
 * If undefined, only error codes like "E12 " will be printed.
 */
#ifndef ARCH_AVR
#define NF_ERROR_TEXT
#endif

#include "nf-platform.h"

#endif /* __NF_DEFS_H__ */
