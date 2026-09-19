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

#include <modxo.h>
#include <modxo/lpc_interface.h>
#include "tusb.h"

uint8_t ien = 0;     // 0x3F9 - Interrupt Enable register
uint8_t iir = 0;     // 0x3FA - Interrupt Ident register
uint8_t lcr = 0;     // 0x3FB - Line Control register
uint8_t mcr = 0;     // 0x3FC - Modem Control register
uint8_t msr = 0;     // 0x3FE - Moden Status register
uint8_t scratch = 0; // 0x3FF - arbitrary scratch register

static void uart_16550_port_write(uint16_t address, uint8_t *data)
{
    if(tud_cdc_connected()) {
        // UART Ports
        if ((address == 0x3F8))
        {
            tud_cdc_write(data, 1);
            tud_cdc_write_flush();
        }

        switch(address) {
            case 0x3F9: ien = *data; break;
            case 0x3FA: iir = *data; break;
            case 0x3FB: lcr = *data; break;
            case 0x3FC: mcr = *data; break;
            case 0x3FF: scratch = *data; break;
        }
    }
}

static void uart_16550_port_read(uint16_t address, uint8_t *data)
{
    // UART Ports
    if (tud_cdc_connected())
    { // If usb serial port is open
        if (address == 0x3FD)
        {
            uint32_t avail = tud_cdc_write_available();
            *data = (0
                | (avail == CFG_TUD_CDC_TX_BUFSIZE ? 0x40 : 0x00)
                | (avail ? 0x20 : 0x00)
                | (tud_cdc_available() ? 0x01 : 0x00)
            );
        }

        if (address == 0x3F8)
        {
            tud_cdc_read(data, 1);
        }

        switch(address) {
            case 0x3F9: *data = ien; break;
            case 0x3FA: *data = iir; break;
            case 0x3FB: *data = lcr; break;
            case 0x3FC: *data = mcr; break;
            case 0x3FE: *data = msr; break;
            case 0x3FF: *data = scratch; break;
        }
    }
    else
    {
        if (address == 0x3FD)
        {
            *data = 0xff; // prevents potential busy-wait infinite loop in kernel
        }
        else
        {
            *data = 0;
        }
    }
}

static void powerup(void)
{
    if(tud_cdc_connected())
    {
        tud_cdc_write_clear();
        tud_cdc_read_flush();
    }
}

static void uart_16550_init(void)
{
    lpc_interface_add_io_handler(0x03F8, 0xFFF8, uart_16550_port_read, uart_16550_port_write); // 16550 Uart port emulation
}

MODXO_TASK uart_16550_hdlr = {
    .init = uart_16550_init,
    .powerup = powerup,
};