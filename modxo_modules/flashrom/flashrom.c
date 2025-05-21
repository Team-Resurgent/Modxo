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
#include "hardware/irq.h"
#include "hardware/structs/bus_ctrl.h"
#include <modxo/lpc_interface.h>
#include <modxo.h>

#include <flashrom.h>
#include <hardware/sync.h>
#include <hardware/flash.h>

#define MODXO_BANK_BOOTLOADER 0x01
#define STORAGE_CMD_TOTAL_BYTES 64



#define MODXO_REGISTER_BANKING   0xDEAA
#define MODXO_REGISTER_SIZE      0xDEAB
#define MODXO_REGISTER_MEM_ERASE 0xDEAC
#define MODXO_REGISTER_MEM_FLUSH 0xDEAE


#define FLASH_WRITE_PAGE_SIZE 4096
#define FLASH_START_ADDRESS ((uint8_t *)0x10000000)
static uint32_t flash_rom_mask;
static uint8_t mmc_register;
static uint8_t * flash_rom_data = NULL;

static uint8_t flash_write_buffer[FLASH_WRITE_PAGE_SIZE];


enum
{
    WAIT_PASSWORD,
    SECTOR_NUMBER,
    SECTOR_ERASING
} erase_password;

static int _erase_sector_number = -1;
static int _program_sector_number = -1;
uint8_t password_index = 0;
uint8_t flash_size = 0;

char password_sequence[] = "DIE";

static void set_mmc(uint8_t);
static uint8_t get_mmc(void);
static void erase_sector(uint8_t sectorn);
static void program_sector(uint8_t sectorn);
static uint8_t get_flash_size(void);
static void reset(void);

static void write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    // Memory Management Controller Registers
    case MODXO_REGISTER_BANKING:
        set_mmc(*data);
        break;

    // Sector Erase
    case MODXO_REGISTER_MEM_ERASE:
        if (password_sequence[password_index] == 0)
        {
            _erase_sector_number = *data;
            password_index = 0;
            __sev();
        }
        else if (password_sequence[password_index] == *data)
        {
            password_index++;
        }
        else
        {
            password_index = 0;
        }

        break;

    // Sector Flush
    case MODXO_REGISTER_MEM_FLUSH:
        _program_sector_number = *data;
        __sev();
        break;

    }
}

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    // Memory Management Controller Registers
    case MODXO_REGISTER_BANKING:
        *data = get_mmc();
        break;
    case MODXO_REGISTER_SIZE:
        *data = get_flash_size();
        break;
    case  MODXO_REGISTER_MEM_ERASE:
        // Is Erasing
        *data = _erase_sector_number < 0 ? false : true;
        break;
    case MODXO_REGISTER_MEM_FLUSH:
        // Is Programming
        *data = _program_sector_number < 0 ? false : true;
        break;
    }  
}

static void flashrom_memread_handler(LPC_OP_TYPE sm, uint32_t address, uint8_t *data)
{
    register uint32_t mem_data;
    *data = flash_rom_data[address & flash_rom_mask];
}

static void flashrom_memwrite_handler(LPC_OP_TYPE sm, uint32_t address, uint8_t *data)
{
    flash_write_buffer[address & (FLASH_WRITE_PAGE_SIZE - 1)] = *data;
}

static void powerup(void)
{
    if (flash_rom_data)
        lpc_interface_mem_global_read_handler(flash_rom_data, flash_rom_mask);
    lpc_interface_mem_global_write_handler(flash_write_buffer, FLASH_WRITE_PAGE_SIZE - 1);

    set_mmc(MODXO_BANK_BOOTLOADER);
    _erase_sector_number = -1;
    _program_sector_number = -1;
    erase_password = WAIT_PASSWORD;
    password_index = 0;
}

static void dummy_read_handler(uint16_t address, uint8_t *data)
{
    *data = 0xFF;
}

static void dummy_write_handler(uint16_t address, uint8_t *data)
{
    // Do nothing
}

static void init(void)
{
    powerup();

    lpc_interface_io_add_handler(MODXO_REGISTER_BANKING, MODXO_REGISTER_BANKING + 1, read_handler, write_handler);
    lpc_interface_io_add_handler(MODXO_REGISTER_MEM_ERASE, MODXO_REGISTER_MEM_ERASE, read_handler, write_handler);
    lpc_interface_io_add_handler(MODXO_REGISTER_MEM_FLUSH, MODXO_REGISTER_MEM_FLUSH, read_handler, write_handler);
    lpc_interface_io_add_handler(0x1900, 0x190F, dummy_read_handler, dummy_write_handler);
}

static void program_sector(uint8_t sector_number)
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

static void erase_sector(uint8_t sector_number)
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

static void set_mmc(uint8_t data)
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

    lpc_interface_mem_global_read_handler(flash_rom_data, flash_rom_mask);
}

static uint8_t get_mmc(void)
{
    return mmc_register;
}

static uint8_t get_flash_size(void)
{
    if (flash_size == 0)
    {
        uint8_t txbuf[STORAGE_CMD_TOTAL_BYTES] = {0x9f};
        uint8_t rxbuf[STORAGE_CMD_TOTAL_BYTES] = {0};
        flash_do_cmd(txbuf, rxbuf, STORAGE_CMD_TOTAL_BYTES);
        flash_size = 1 << (rxbuf[3] - 20);
    }
    return flash_size;
}

static void core1_poll(void)
{
    uint32_t val;
    if (_erase_sector_number >= 0)
    {
        erase_sector(_erase_sector_number);
        _erase_sector_number = -1;
    }

    if (_program_sector_number >= 0)
    {
        program_sector(_program_sector_number);
        _program_sector_number = -1;
    }
}


MODXO_TASK flashrom_hdlr = {
    .init = init,
    .powerup = powerup,
    .core1_poll = core1_poll
};
