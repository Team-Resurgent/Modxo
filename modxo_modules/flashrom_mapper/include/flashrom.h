/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#ifndef _FLASH_MEM_H_
#define _FLASH_MEM_H_

#include <modxo.h>
#include <stdint.h>

uint8_t flashrom_memread_handler  (uint32_t address);
void    flashrom_memwrite_handler (uint32_t address, uint8_t data);

extern MODXO_TASK flashrom_hdlr;
#endif