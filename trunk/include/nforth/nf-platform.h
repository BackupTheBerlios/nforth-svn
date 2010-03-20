/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    Platform-specific defines for nForth
*/

#ifndef __NF_PLATFORM_H__
#define __NF_PLATFORM_H__

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

#if defined ARCH_AVR
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

// Align functions to this boundary
# define __FUNC_ALIGN	2
# define NF_ALIGN_EOF	_NF_EMIT_ALIGN (__FUNC_ALIGN);

# define NF_RAM_SIZE	(RAMEND+1)
# define NF_EEPROM_SIZE	(E2END+1)

// Always keep data stack pointer in the Y register
// (the only of X,Y,Z which is call-saved by default)
# define __R_DSP	"r28"
// And the indirect code pointer in r16
// (which is call-saved and accessible by word-sized instructions)
# define __R_ICP	"r16"

// using asm statements here gives slightly better code
#  define nf_push(n)	__asm volatile ("st Y+,%A0\n\tst Y+,%B0" : : "r" ((cell_t)n))
#  define nf_pop()	({ cell_t res; __asm volatile ("ld %B0,-Y\n\tld %A0,-Y" : "=r" (res)); res; })

#  define NF_FLASH
#  define _NF_FLASH_SEC
#  define NF_EEPROM	EEMEM
#  define NF_RAM

// This type represents an integer with the size like a pointer
typedef unsigned short uintptr;

#else

// The amount of "simulated" RAM, defined in linker script file
# define NF_RAM_SIZE	16384
// The amount of "simulated" EEPROM, defined in linker script file
# define NF_EEPROM_SIZE	16384

# define __FUNC_ALIGN	16

#if defined ARCH_X86_64
// This register will be used to store a pointer to current context
# define __R_DSP	"r12"
// Indirect code pointer
# define __R_ICP	"r13"

typedef unsigned long uintptr;

#elif defined ARCH_X86
# define __R_DSP	"esi"
# define __R_ICP	"edi"

typedef unsigned long uintptr;

#else
# error "Target architecture is not yet supported"
#endif

#endif

#ifndef NF_ALIGN_EOF
# define NF_ALIGN_EOF
#endif

#ifndef NF_FLASH
// Attribute for functions in flash memory
# define NF_FLASH		__attribute__((section(".nf_flash")))
# if defined TARGET_LINUX
// This option is valid for ELF output format only
#  define _NF_FLASH_SEC		".section .nf_flash,\"ax\",@progbits"
# elif defined TARGET_WINDOWS
#  define _NF_FLASH_SEC		".section .nf_flash,\"x\""
# else
#  define _NF_FLASH_SEC		".section .nf_flash"
# endif
#endif

#ifdef TARGET_WINDOWS
# define _NF_ASMNAME(x)		_NF_JOIN(_,x)
// No weak symbol support in mingw32
# define NF_WEAK
#else
# define _NF_ASMNAME(x)		x
#endif

#ifndef NF_WEAK
# define NF_WEAK		__attribute__((weak))
#endif

#ifndef __CODE_END
// This label marks the start of free flash memory
#define __CODE_END		__flash_end
#endif
#ifndef __EEPROM_END
// This label marks the start of free EEPROM
#define __EEPROM_END		__eeprom_end
#endif
#ifndef __RAM_END
// This label marks the start of free RAM
#define __RAM_END		__ram_end
#endif

extern void __flash_end, __eeprom_end, __ram_end;

#ifndef NF_RAM
// Attribute for variables in RAM
# define NF_RAM         	__attribute__((section(".nf_ram")))
#endif

#ifndef NF_EEPROM
// Attribute for variables in EEPROM
# define NF_EEPROM		__attribute__((section(".nf_eeprom")))
#endif

#define NF_CODE2ADDR(offs)	((uintptr)offs - __CODE_BASE)
#define NF_ADDR2CODE(addr)	((uintptr)addr + __CODE_BASE)
#define NF_RAM2ADDR(offs)	((uintptr)offs - __RAM_BASE + NF_RAM_ADDR)
#define NF_ADDR2RAM(addr)	((uintptr)addr + __RAM_BASE - NF_RAM_ADDR)
#define NF_EEPROM2ADDR(offs)	((uintptr)offs - __EEPROM_BASE + NF_EEPROM_ADDR)
#define NF_ADDR2EEPROM(addr)	((uintptr)addr + __EEPROM_BASE - NF_EEPROM_ADDR)

#endif /* __NF_PLATFORM_H__ */
