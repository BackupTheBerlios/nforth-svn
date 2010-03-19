/*
    The nForth language interpreter
    Copyright (C) 2010 Andrew Zabolotny <zap@cobra.ru>

    nForth system-specific support
*/

#include "../nf-support.h"
#include <avr/pgmspace.h>

#include <stdio.h>

uint8_t nf_readb (addr_t addr)
{
    if (addr < NF_EEPROM_ADDR)
        return pgm_read_byte_near (addr);
    addr -= NF_EEPROM_ADDR;
    if (addr < NF_EEPROM_SIZE)
        return eeprom_read_byte ((uint8_t *)addr);
    addr -= NF_EEPROM_SIZE;
    return *(uint8_t *)addr;
}

uint16_t nf_readw (addr_t addr)
{
    if (addr < NF_EEPROM_ADDR)
        return pgm_read_word_near (addr);
    addr -= NF_EEPROM_ADDR;
    if (addr < NF_EEPROM_SIZE)
        return eeprom_read_word ((uint16_t *)addr);
    addr -= NF_EEPROM_SIZE;
    return *(uint16_t *)addr;
}

void nf_writeb (addr_t addr, uint8_t val)
{
    if (addr < NF_EEPROM_ADDR)
    {
        // ROM is not writable
        return;
    }
    addr -= NF_EEPROM_ADDR;
    if (addr < NF_EEPROM_SIZE)
    {
        eeprom_write_byte ((uint8_t *)addr, val);
        return;
    }
    addr -= NF_EEPROM_SIZE;
    *(uint8_t *)addr = val;
}

void nf_writew (addr_t addr, uint16_t val)
{
    if (addr < NF_EEPROM_ADDR)
    {
        // ROM is not writable
        return;
    }
    addr -= NF_EEPROM_ADDR;
    if (addr < NF_EEPROM_SIZE)
    {
        eeprom_write_word ((uint16_t *)addr, val);
        return;
    }
    addr -= NF_EEPROM_SIZE;
    *(uint16_t *)addr = val;
}
