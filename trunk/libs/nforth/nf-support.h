/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth system-specific support
*/

#ifndef __NF_SUPPORT_H__
#define __NF_SUPPORT_H__

#include "nf.h"

// --- DEBUG TRACING ---

#ifdef __DEBUG__
//# define NF_TRACE
#endif

#ifdef NF_TRACE
#  define NF_TRACE_WORD(comment,word) \
    nf_trace_word (comment, word)
#  define NF_TRACE_LEVEL_INC nf_trace_level++
#  define NF_TRACE_LEVEL_DEC nf_trace_level--
extern void nf_trace_word (const char *comment, addr_t word);
extern int nf_trace_level;
#else
#  define NF_TRACE_WORD(comment,word)
#  define NF_TRACE_LEVEL_INC
#  define NF_TRACE_LEVEL_DEC
#endif

// Low-level support routines ...

#if defined ARCH_AVR

/// Similar to setjmp(): save current execution context
# define nf_catch() \
    ({ \
        register uintptr tmp __asm ("r24"); \
        __asm volatile ( \
            "ld    %A0,X+\n" \
            "ld    %B0,X\n" \
            "push  %B0\n" \
            "push  %A0\n" \
            "push  r17\n" \
            "push  r16\n" \
            "sbiw  r28,2\n" \
            "push  r29\n" \
            "push  r28\n" \
            "adiw  r28,2\n" \
            "in    %A0,__SP_L__\n" \
            "in    %B0,__SP_H__\n" \
            "st    X,%B0\n" \
            "st    -X,%A0\n" \
            "rcall EXECUTE\n" \
            "clr   %A0\n" \
            "clr   %B0\n" \
        "L_ret_catch:\n" \
            : "=&r" (tmp) : "x" (&nf_ctx->catch_ptr)); \
        nf_push (tmp); \
    })

/// Similar to longjmp(): restore execution context saved by nf_catch()
# define nf_throw(n) \
    { \
        register uintptr _n asm ("r24") = n; \
        uintptr tmp; \
        __asm volatile ( \
            "ld    %A0,X+\n" \
            "ld    %B0,X\n" \
            "in    __tmp_reg__,__SREG__\n" \
            "cli\n" \
            "out   __SP_H__,%B0\n" \
            "out   __SREG__,__tmp_reg__\n" \
            "out   __SP_L__,%A0\n" \
            "pop   r28\n" \
            "pop   r29\n" \
            "pop   r16\n" \
            "pop   r17\n" \
            "pop   %A0\n" \
            "pop   %B0\n" \
            "st    X,%B0\n" \
            "st    -X,%A0\n" \
            "rjmp  L_ret_catch\n" \
            : "=&r" (tmp) : "x" (&nf_ctx->catch_ptr), "r" (_n)); \
    }

# define nf_call(addr) \
    __asm volatile ("adiw r30,1\n" \
                    "lsr  r31\n" \
                    "ror  r30\n" \
                    "icall\n" \
                    : : "z" (addr));

#elif defined ARCH_X86_64

# define nf_catch() \
    __asm volatile ( \
        "pushq $1f\n" \
        "pushq (%0)\n" \
        "pushq %%rbx\n" \
        "pushq %%rbp\n" \
        "pushq %%r14\n" \
        "pushq %%r15\n" \
        "pushq %%"__R_ICP"\n" \
        "pushq %%"__R_DSP"\n" \
        "subq  $2,(%%rsp)\n" \
        "movq  %%rsp,(%0)\n" \
        "call  EXECUTE\n" \
        "xorq  %0,%0\n" \
    "1:  movw  %0,(%%"__R_DSP")\n" \
        "addq  $2,%%"__R_DSP"\n" \
        : : "a" (&nf_ctx->catch_ptr) : \
          "rcx", "rdx", "rsi", "rdi", "r8", "r9", "r10", /*"r11",*/ "r12", "r13", "memory");

# define nf_throw(n) \
    __asm volatile ( \
        "movq (%1),%%rsp\n" \
        "popq %%"__R_DSP"\n" \
        "popq %%"__R_ICP"\n" \
        "popq %%r15\n" \
        "popq %%r14\n" \
        "popq %%rbp\n" \
        "popq %%rbx\n" \
        "popq (%1)\n" \
        "retq\n" \
        : : "a" (n), "r" (&nf_ctx->catch_ptr)); \

#elif defined ARCH_X86

# define nf_catch() \
    __asm volatile ( \
        "pushl $1f\n" \
        "pushl (%0)\n" \
        "pushl %%ebx\n" \
        "pushl %%ebp\n" \
        "pushl %%"__R_ICP"\n" \
        "pushl %%"__R_DSP"\n" \
        "subl  $2,(%%esp)\n" \
        "movl  %%esp,(%0)\n" \
        "call  " _NF_STRINGIFY (_NF_ASMNAME (EXECUTE))"\n" \
        "xorl  %0,%0\n" \
    "1:  movw  %%ax,(%%"__R_DSP")\n" \
        "addl  $2,%%"__R_DSP"\n" \
        : : "a" (&nf_ctx->catch_ptr) : \
          "ecx", "edx", "memory");

# define nf_throw(n) \
    __asm volatile ( \
        "movl (%1),%%esp\n" \
        "popl %%"__R_DSP"\n" \
        "popl %%"__R_ICP"\n" \
        "popl %%ebp\n" \
        "popl %%ebx\n" \
        "popl (%1)\n" \
        "retl\n" \
        : : "a" (n), "r" (&nf_ctx->catch_ptr)); \

#else
# error "Target architecture is not yet supported"
#endif

#ifndef nf_call
# define nf_call(addr) \
    /* Align the function address according to platform-specific rules */ \
    tmp = (tmp + __FUNC_ALIGN - 1) & ~(__FUNC_ALIGN - 1); \
    ((void (*) ())(NF_ADDR2CODE (tmp))) ();
#endif

#ifndef nf_xchg
# define nf_xchg(v1, v2) \
    { \
        uint16_t __tmp__ = v1; \
        v1 = v2; v2 = __tmp__; \
    }
#endif

/// This macro returns the offset of a field within a structure
#define MEMBER_OFFSET(s,f) ((uintptr)&((s *)0)->f)

extern void nf_memcpy (addr_t dst, addr_t src, uint8_t n);
extern cell_t nf_strlen (addr_t str);

#endif /* __NF_SUPPORT_H__ */
