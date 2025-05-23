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
#include <modxo/modxo_ports.h>
#include "tusb.h"

#define UART_ADDR_MASK      0xFF00
#define UART_1_ITF          1
#define UART_2_ITF          2

static void uart_16550_port_read(uint8_t itf, uint8_t port, uint8_t *data)
{
    switch (port)
    {
    case 0xFD:
        *data = tud_cdc_n_connected(itf)
            ? (tud_cdc_n_write_available(itf) ? 0x20 : 0x00) | (tud_cdc_n_available(itf) ? 0x01 : 0x00)
            : 0xFF; // prevents potential busy-wait infinite loop in kernel
        break;
    case 0xF8:
        if (tud_cdc_n_connected(itf)) tud_cdc_n_read(itf, data, 1);
        else *data = 0;
        break;
    }
}

static void uart_16550_port_write(uint8_t itf, uint8_t port, uint8_t *data)
{
    switch (port)
    {
    case 0xF8:
        if (tud_cdc_n_connected(itf))
        {
            tud_cdc_n_write(itf, data, 1);
            tud_cdc_n_write_flush(itf);
        }
        break;
    }
}

void uart_16550_com1_read(uint16_t address, uint8_t * data)
{
    uart_16550_port_read(UART_1_ITF, (uint8_t)address, data);
}

void uart_16550_com1_write(uint16_t address, uint8_t * data)
{
    uart_16550_port_write(UART_1_ITF, (uint8_t)address, data);
}

void uart_16550_com2_read(uint16_t address, uint8_t * data)
{
    uart_16550_port_read(UART_2_ITF, (uint8_t)address, data);
}

void uart_16550_com2_write(uint16_t address, uint8_t * data)
{
    uart_16550_port_write(UART_2_ITF, (uint8_t)address, data);
}

static void powerup(void)
{
    if(tud_cdc_n_connected(1))
    {
        tud_cdc_n_write_clear(1);
        tud_cdc_n_read_flush(1);
    }

    if(tud_cdc_n_connected(2))
    {
        tud_cdc_n_write_clear(2);
        tud_cdc_n_read_flush(2);
    }
}

MODXO_TASK uart_16550_hdlr = {
    .powerup = powerup,
};
