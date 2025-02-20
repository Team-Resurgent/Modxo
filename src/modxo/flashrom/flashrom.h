/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#ifndef _FLASH_MEM_H_
#define _FLASH_MEM_H_

#include <stdint.h>

#define MODXO_BANK_BOOTLOADER 0x01

void flashrom_reset(void);
bool flashrom_init(void);
void flashrom_set_mmc(uint8_t);
uint8_t flashrom_get_mmc(void);
void flashrom_erase_sector(uint8_t sectorn);
void flashrom_program_sector(uint8_t sectorn);

#endif