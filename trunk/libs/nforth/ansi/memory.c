/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth memory access routines
*/

#include "../nf-support.h"

/*
 * Platform-specific code for von Neumann architectures
 */

uint8_t nf_readb (addr_t addr)
{
    if (addr < NF_EEPROM_ADDR)
        return *(uint8_t *)((uintptr)__CODE_BASE + addr);
    if (addr < NF_EEPROM_ADDR + NF_EEPROM_SIZE)
        return *(uint8_t *)((uintptr)__EEPROM_BASE - NF_EEPROM_ADDR + addr);;
    return *(uint8_t *)((uintptr)__RAM_BASE - NF_EEPROM_ADDR - NF_EEPROM_SIZE + addr);
}

uint16_t nf_readw (addr_t addr)
{
    if (addr < NF_EEPROM_ADDR)
        return *(uint16_t *)((uintptr)__CODE_BASE + addr);
    if (addr < NF_EEPROM_ADDR + NF_EEPROM_SIZE)
        return *(uint16_t *)((uintptr)__EEPROM_BASE - NF_EEPROM_ADDR + addr);
    return *(uint16_t *)((uintptr)__RAM_BASE - NF_EEPROM_ADDR - NF_EEPROM_SIZE + addr);
}

void nf_writeb (addr_t addr, uint8_t val)
{
    if (addr < NF_EEPROM_ADDR)
    {
        // ROM is not writable
        //*(uint8_t *)((uintptr)__CODE_BASE + addr) = val;
        return;
    }
    if (addr < NF_EEPROM_ADDR + NF_EEPROM_SIZE)
    {
        *(uint8_t *)((uintptr)__EEPROM_BASE - NF_EEPROM_ADDR + addr) = val;
        return;
    }
    *(uint8_t *)((uintptr)__RAM_BASE - NF_EEPROM_ADDR - NF_EEPROM_SIZE + addr) = val;
}

void nf_writew (addr_t addr, uint16_t val)
{
    if (addr < NF_EEPROM_ADDR)
    {
        // ROM is not writable
        //*(uint16_t *)(__CODE_BASE + addr) = val;
        return;
    }
    if (addr < NF_EEPROM_ADDR + NF_EEPROM_SIZE)
    {
        *(uint16_t *)((uintptr)__EEPROM_BASE - NF_EEPROM_ADDR + addr) = val;
        return;
    }
    *(uint16_t *)((uintptr)__RAM_BASE - NF_EEPROM_ADDR - NF_EEPROM_SIZE + addr) = val;
}
