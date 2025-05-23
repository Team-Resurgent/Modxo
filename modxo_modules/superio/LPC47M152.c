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
#define CONFIG_ADDRESS_H 0x00
#define CONFIG_ADDRESS_L 0x2E

static struct
{
    bool config_mode;
    uint8_t index_port;
    uint8_t device_id;
} lpc47m152_regs;

static void lpc47m152_write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case 0x002E:
        if (lpc47m152_regs.config_mode == false)
        {
            if (*data == ENTER_CONFIGURATION_MODE_VALUE)
            {
                lpc47m152_regs.config_mode = true;
            }
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
        break;

    case 0x002F:
        // Not used
        /*          if(lpc47m152_regs.config_mode == true)
                    {
                        switch(lpc47m152_regs.index_port)
                        {

                        }
                    }
        */
        break;
    }
}

static void lpc47m152_read_handler(uint16_t address, uint8_t *data)
{
    if (lpc47m152_regs.config_mode)
    {
        switch (address)
        {
        case 0x2E:
            *data = lpc47m152_regs.index_port;
            break;
        case 0x2F:
            switch (lpc47m152_regs.index_port)
            {
            case 0x26:
                *data = tud_cdc_connected() ? CONFIG_ADDRESS_L : 0xff;
                break;
            case 0x27:
                *data = tud_cdc_connected() ? CONFIG_ADDRESS_H : 0xff;
                break;
            }
            break;
        }
    }
}

static bool superio_connected(void)
{
    return tud_cdc_connected();
}

static void powerup(void) {
    lpc47m152_regs.config_mode = false;
    lpc47m152_regs.index_port = 0;
    lpc47m152_regs.device_id = 0;
    uart_16550_hdlr.powerup();
}

static void lpc47m152_init(void)
{
    modxo_debug_sp_connected = superio_connected;
    powerup();
    lpc_interface_add_io_handler(0x002E, 0xFFFE, lpc47m152_read_handler, lpc47m152_write_handler); // LPC47M152(superio) port emulation
}

MODXO_TASK LPC47M152_hdlr = {
    .init = lpc47m152_init,
    .powerup = powerup,
};
