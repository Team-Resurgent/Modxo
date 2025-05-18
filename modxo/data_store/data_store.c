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

#define DATA_STORE_PORT_BASE MODXO_REGISTER_VOLATILE_CONFIG_SEL
#define DATA_STORE_COMMAND_PORT DATA_STORE_PORT_BASE
#define DATA_STORE_DATA_PORT DATA_STORE_PORT_BASE + 1

#define DATA_STORE_ADDR_START   0xFF800000
#define DATA_STORE_ADDR_END     0xFF8FFFFF
#define DATA_STORE_ADDR_MASK    0x1FFF // 8KB is the smallest size Windows can mount

static uint8_t io_buffer[256];
static uint8_t io_buffer_pos;

#define MEM_BUFFER_SIZE (DATA_STORE_ADDR_MASK + 1) // 8K
static uint8_t mem_buffer[MEM_BUFFER_SIZE];

static void lpc_io_read(uint16_t address, uint8_t *data)
{
    if (address == DATA_STORE_DATA_PORT)
    {
        *data = io_buffer[io_buffer_pos];
    }
}

static void lpc_io_write(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case DATA_STORE_COMMAND_PORT:
        io_buffer_pos = *data;
        break;
    case DATA_STORE_DATA_PORT:
        io_buffer[io_buffer_pos] = *data;
        break;
    }
}

static void lpc_mem_read(uint32_t address, uint8_t *data)
{
    *data = mem_buffer[address & DATA_STORE_ADDR_MASK];
}

static void lpc_mem_write(uint32_t address, uint8_t *data)
{
    mem_buffer[address & DATA_STORE_ADDR_MASK] = *data;
}

static int32_t msc_read(uint32_t block, uint32_t offset, uint8_t *data, uint32_t size)
{
    uint32_t address = block * MSC_DEFAULT_BLOCK_SIZE + offset;
    if (address >= MEM_BUFFER_SIZE) return -1;

    // Don't overshoot the end of the buffer
    int32_t consumed = address + size > MEM_BUFFER_SIZE ? MEM_BUFFER_SIZE - address : size;

    memcpy(data, mem_buffer + address, consumed);

    return consumed;
}

static int32_t msc_write(uint32_t block, uint32_t offset, uint8_t *data, uint32_t size)
{
    uint32_t address = block * MSC_DEFAULT_BLOCK_SIZE + offset;
    if (address >= MEM_BUFFER_SIZE) return -1;

    // Don't overshoot the end of the buffer
    int32_t consumed = address + size > MEM_BUFFER_SIZE ? MEM_BUFFER_SIZE - address : size;

    memcpy(mem_buffer + address, data, consumed);

    return consumed;
}

static void powerup(void)
{
    io_buffer_pos = 0;
    memset(io_buffer, 0, sizeof(io_buffer));
    memset(mem_buffer, 0 , sizeof(mem_buffer));
}

static void init(void)
{
    powerup();
    lpc_interface_add_io_handler(DATA_STORE_COMMAND_PORT, DATA_STORE_DATA_PORT, lpc_io_read, lpc_io_write);
    //lpc_interface_add_mem_handler(DATA_STORE_ADDR_START, DATA_STORE_ADDR_END, lpc_mem_read, lpc_mem_write);
    msc_interface_add_handler(MEM_BUFFER_SIZE / MSC_DEFAULT_BLOCK_SIZE, MSC_DEFAULT_BLOCK_SIZE, "Mailbox", msc_read, msc_write);
}

MODXO_TASK data_store_handler = {
    .init = init,
    .powerup = powerup,
};