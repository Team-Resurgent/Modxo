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

#include "../lpc/lpc_interface.h"
#include "../lpc/lpc_regs.h"
#include "../modxo_ports.h"
#include "math.h"

#define DATA_STORE_PORT_BASE MODXO_REGISTER_DATA_STORE_COMMAND
#define DATA_STORE_ADDRESS_MASK 0xFE
#define DATA_STORE_COMMAND_PORT DATA_STORE_PORT_BASE
#define DATA_STORE_DATA_PORT DATA_STORE_PORT_BASE + 1

uint8_t data_store_cmd;
uint8_t data_store_buffer[256];

static void lpc_port_read(uint16_t address, uint8_t *data)
{
    if (address == DATA_STORE_DATA_PORT)
    {
        *data = data_store_buffer[data_store_cmd];
    }
}

static void lpc_port_write(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case DATA_STORE_COMMAND_PORT:
        data_store_cmd = *data;
        break;
    case DATA_STORE_DATA_PORT:
        data_store_buffer[data_store_cmd] = *data;
        break;
    }
}

void data_store_init()
{
    data_store_cmd = 0;
    memset(data_store_buffer, 0, sizeof(data_store_buffer));

    lpc_interface_add_io_handler(DATA_STORE_PORT_BASE, DATA_STORE_ADDRESS_MASK, lpc_port_read, lpc_port_write);
}