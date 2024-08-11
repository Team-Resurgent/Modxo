/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "hardware/structs/bus_ctrl.h"
#include "../lpc/lpc_interface.h"

#include "flashrom.h"
#include <hardware/sync.h>
#include <hardware/flash.h>

#define FLASH_WRITE_PAGE_SIZE 4096
#define FLASH_START_ADDRESS ((uint8_t *)0x10000000)
static uint32_t flash_rom_mask;
static uint8_t mmc_register;
static uint8_t *flash_rom_data;

static uint8_t flash_write_buffer[FLASH_WRITE_PAGE_SIZE];

static void flashrom_memread_handler(uint32_t address, uint8_t *data)
{
    register uint32_t mem_data;
    *data = flash_rom_data[address & flash_rom_mask];
}

static void flashrom_memwrite_handler(uint32_t address, uint8_t *data)
{
    flash_write_buffer[address & (FLASH_WRITE_PAGE_SIZE - 1)] = *data;
}

void flashrom_program_sector(uint8_t sector_number)
{
    uint32_t flash_offset;
    uint32_t ints = save_and_disable_interrupts(); // Add this
    uint8_t size = (mmc_register >> 6);
    uint32_t bank_size = 1 << (18 + size);
    uint8_t bank_number = mmc_register & 0x3F;
    flash_offset = bank_size * bank_number + sector_number * FLASH_WRITE_PAGE_SIZE;
    flash_range_program(flash_offset, flash_write_buffer, FLASH_WRITE_PAGE_SIZE);
    restore_interrupts(ints); // Add this
}

void flashrom_erase_sector(uint8_t sector_number)
{
    uint32_t flash_offset;
    uint32_t ints = save_and_disable_interrupts(); // Add this
    uint8_t size = (mmc_register >> 6);
    uint32_t bank_size = 1 << (18 + size);
    uint8_t bank_number = mmc_register & 0x3F;
    flash_offset = bank_size * bank_number + sector_number * FLASH_WRITE_PAGE_SIZE;
    flash_range_erase(flash_offset, FLASH_WRITE_PAGE_SIZE);
    restore_interrupts(ints); // Add this
}

void flashrom_set_mmc(uint8_t data)
{
    mmc_register = data;
    uint8_t size = (data >> 6);
    bool enableMod = size < 3;
    lpc_interface_disable_onboard_flash(enableMod);
    if (enableMod == false)
    {
        return;
    }
    uint32_t bank_size = 1 << (18 + size);
    uint8_t bank_number = data & 0x3F;
    flash_rom_mask = bank_size - 1;
    flash_rom_data = FLASH_START_ADDRESS + bank_size * bank_number;
}

uint8_t flashrom_get_mmc(void)
{
    return mmc_register;
}

bool flashrom_init(void)
{
    flashrom_set_mmc(MODXO_BANK_BOOTLOADER);
    lpc_interface_set_callback(LPC_OP_MEM_READ, flashrom_memread_handler);
    lpc_interface_set_callback(LPC_OP_MEM_WRITE, flashrom_memwrite_handler);

    return (flash_rom_mask != 0xFFFFFFFF);
}
