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
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#include <modxo/lpc_interface.h>
#include <modxo/lpc_regs.h>
#include <modxo.h>
#include <modxo/modxo_ports.h>
#include <modxo/msc_interface.h>

static uint8_t io_buffer[256];
static uint8_t io_buffer_idx;

void data_store_idx_read(uint16_t address, uint8_t * data)
{
    *data = io_buffer_idx;
}

void data_store_idx_write(uint16_t address, uint8_t * data)
{
    io_buffer_idx = *data;
}

void data_store_val_read(uint16_t address, uint8_t * data)
{
    *data = io_buffer[io_buffer_idx];
}

void data_store_val_write(uint16_t address, uint8_t * data)
{
    io_buffer[io_buffer_idx] = *data;
}

#define MAILBOX_ADDR 0xFF800000
#define MAILBOX_SIZE 0x2000 // 8KB is the smallest size Windows can mount
static uint8_t mailbox_buffer[MAILBOX_SIZE];

static int32_t msc_read(uint32_t block, uint32_t offset, uint8_t *data, uint32_t size)
{
    uint32_t address = block * MSC_DEFAULT_BLOCK_SIZE + offset;
    if (address >= MAILBOX_SIZE) return -1;

    // Don't overshoot the end of the buffer
    int32_t consumed = address + size > MAILBOX_SIZE ? MAILBOX_SIZE - address : size;

    memcpy(data, mailbox_buffer + address, consumed);

    return consumed;
}

static int32_t msc_write(uint32_t block, uint32_t offset, uint8_t *data, uint32_t size)
{
    uint32_t address = block * MSC_DEFAULT_BLOCK_SIZE + offset;
    if (address >= MAILBOX_SIZE) return -1;

    // Don't overshoot the end of the buffer
    int32_t consumed = address + size > MAILBOX_SIZE ? MAILBOX_SIZE - address : size;

    memcpy(mailbox_buffer + address, data, consumed);

    return consumed;
}

static void powerup(void)
{
    io_buffer_idx = 0;
    memset(io_buffer, 0, sizeof(io_buffer));
    memset(mailbox_buffer, 0 , sizeof(mailbox_buffer));
}

static void init(void)
{
    powerup();
    /*
    uint32_t address = MAILBOX_ADDR;
    for (uint i = 0; i < 8; i++)
    {
        lpc_interface_mem_set_read_handler(address,  mailbox_buffer, MAILBOX_SIZE - 1);
        lpc_interface_mem_set_write_handler(address, mailbox_buffer, MAILBOX_SIZE - 1);
        address += 0x100000;
    }
    */

    msc_interface_add_handler(MAILBOX_SIZE/ MSC_DEFAULT_BLOCK_SIZE, MSC_DEFAULT_BLOCK_SIZE, "Mailbox", msc_read, msc_write);
}

MODXO_TASK data_store_handler = {
    .init = init,
    .powerup = powerup,
};