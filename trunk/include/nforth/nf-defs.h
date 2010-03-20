/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth interpreter: interpreter configuration
*/

#ifndef __NF_DEFS_H__
#define __NF_DEFS_H__

/**
 * Data (+return in multi-task configs) stack size on this platform (in bytes).
 * Data stack grows upwards while return stack grows downwards, thus the
 * memory is used effectively (unless stacks meet somewhere, which whill
 * lead to an almost guaranteed crash, heh).
 *
 * This is the biggest member of the nf_context_t structure. If you're going
 * to use multiple Forth contexts (for multitasking), you must make sure that
 * microcontroller RAM can hold as much nf_context_t structures as you need.
 * Tweaking NF_STACK_SIZE will help you achieve that.
 *
 * Also keep in mind, that Forth variables (ALLOT, VARIABLE) will be allocated
 * from RAM as well. If you fill the whole RAM with a nf_context_t, you won't have
 * space for variables. So you have to make a balanced choice here.
 *
 * Note that NF_STACK_SIZE doesn't have to be a power of two, it's just that
 * I like round numbers :-)
 */
#ifdef ARCH_AVR
#define NF_STACK_SIZE	128
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
 * If defined, nano-Forth kernel will get substantialy larger.
 */
#ifndef ARCH_AVR
#define NF_ERROR_TEXT
#endif

#include "nf-platform.h"

#endif /* __NF_DEFS_H__ */
