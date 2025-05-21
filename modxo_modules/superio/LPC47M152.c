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
#include "pico/multicore.h"
#include "hardware/structs/bus_ctrl.h"
#include <modxo/modxo_debug.h>

#include <modxo/lpc_interface.h>
#include <uart_16550.h>
#include "tusb.h"

#define ENTER_CONFIGURATION_MODE_VALUE 0x55
#define EXIT_CONFIGURATION_MODE_VALUE 0xAA

#if LPC47M152_SYSOPT
    #define CONFIG_PORT_DEFAULT_ADDR 0x004E
#else
    #define CONFIG_PORT_DEFAULT_ADDR 0x002E
#endif

static struct
{
    uint16_t config_port_addr;
    int8_t lpc_hdlr_idx;
    bool config_mode;
    uint8_t index_port;
    uint8_t device_id;
} lpc47m152_regs;

static inline bool lpc_update_io_address(void)
{
    return lpc_interface_io_set_addr(lpc47m152_regs.lpc_hdlr_idx, lpc47m152_regs.config_port_addr, lpc47m152_regs.config_port_addr + 1);
}

static void lpc47m152_write_handler(uint16_t address, uint8_t *data)
{
    if (!lpc47m152_regs.config_mode)
    {
        if (address == lpc47m152_regs.config_port_addr && *data == ENTER_CONFIGURATION_MODE_VALUE)
        {
            lpc47m152_regs.config_mode = true;
        }
        return;
    }

    if (address == lpc47m152_regs.config_port_addr)
    {
        if (*data == EXIT_CONFIGURATION_MODE_VALUE)
        {
            lpc47m152_regs.config_mode = false;
        }
        else
        {
            if (*data == EXIT_CONFIGURATION_MODE_VALUE)
            {
                lpc47m152_regs.config_mode = false;
                uart_16550_hdlr.powerup();
            }
            else
            {
                lpc47m152_regs.index_port = *data;
            }
        }

        return;
    }
    else if (address != lpc47m152_regs.config_port_addr + 1) return;

    switch (lpc47m152_regs.index_port)
    {
    case 0x07:
        lpc47m152_regs.device_id = *data;
        break;
    case 0x26:
        lpc47m152_regs.config_port_addr = (lpc47m152_regs.config_port_addr & 0xFF00) | (uint16_t)(*data);
        lpc_update_io_address();
        break;
    case 0x27:
        lpc47m152_regs.config_port_addr = ((uint16_t)(*data) << 8) | (lpc47m152_regs.config_port_addr & 0xFF);
        lpc_update_io_address();
        break;
    }

    switch (lpc47m152_regs.device_id)
    {
        
    }
}

static void lpc47m152_read_handler(uint16_t address, uint8_t *data)
{
    if (!lpc47m152_regs.config_mode) return;
    
    if (address == lpc47m152_regs.config_port_addr)
    {
        *data = lpc47m152_regs.index_port;
        return;
    }
    else if (address != lpc47m152_regs.config_port_addr + 1) return;

    switch (lpc47m152_regs.index_port)
    {
    case 0x03:
        *data = 0; // Reserved, always returns 0
        break;
    case 0x07:
        *data = lpc47m152_regs.device_id;
        break;
    case 0x20:
        *data = 0x60; // This is the lpc47m152's hard-coded device ID
        break;
    case 0x21:
        *data = 1; // This is the lpc47m152's hard-coded revision ID
        break;
    case 0x26:
        *data = lpc47m152_regs.config_port_addr;
        break;
    case 0x27:
        *data = lpc47m152_regs.config_port_addr >> 8;
        break;
    }

    switch (lpc47m152_regs.device_id)
    {

    }
}

static bool superio_connected(void)
{
    return tud_cdc_n_connected(1);
}

static void powerup(void) {
    lpc47m152_regs.config_port_addr = CONFIG_PORT_DEFAULT_ADDR;
    lpc_update_io_address();

    lpc47m152_regs.config_mode = false;
    lpc47m152_regs.index_port = 0;
    lpc47m152_regs.device_id = 0;
    uart_16550_hdlr.powerup();
}

static void lpc47m152_init(void)
{
    modxo_debug_sp_connected = superio_connected;
    
    lpc47m152_regs.lpc_hdlr_idx = lpc_interface_io_add_handler(CONFIG_PORT_DEFAULT_ADDR, CONFIG_PORT_DEFAULT_ADDR + 1, lpc47m152_read_handler, lpc47m152_write_handler); // LPC47M152(superio) port emulation
    powerup();
}

MODXO_TASK LPC47M152_hdlr = {
    .init = lpc47m152_init,
    .powerup = powerup,
};
