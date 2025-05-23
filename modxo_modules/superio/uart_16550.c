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

#define UART_BREAK_MIN_DURATION_MS 250
#define UART_ADDR_MASK      0xFF00
#define UART_1_ITF          1
#define UART_2_ITF          2

struct {
    bool cts, cts_changed; // This is controlled by RTS
    bool dsr, dsr_changed; // This is controlled by DTR
    bool did_break;
    uint8_t rx_byte;
} com_ports[2];

static void uart_16550_port_read(uint8_t itf, uint8_t port, uint8_t *data)
{
    cdc_line_coding_t coding;
    switch (port)
    {
    case 0xF8:
        // Attemt to read a byte, if this fails, then rx_byte will be unchanged
        tud_cdc_n_read(itf, com_ports[itf - 1].rx_byte, 1);

        *data = com_ports[itf - 1].rx_byte;
        break;
    case 0xFB:
        tud_cdc_n_get_line_coding(itf, &coding);
        *data = ((coding.data_bits - 5) & 3) | (coding.stop_bits ? 4 : 0) | (coding.parity ? 8 : 0) | (((coding.parity - 1) & 3) << 4);
        break;
    case 0xFD:
        *data = (tud_cdc_n_available(itf) ? 0x01 : 0x00) | (com_ports[itf - 1].did_break ? 0x10 : 0x00) | (tud_cdc_n_write_available(itf) ? 0x20 : 0x00);
        
        com_ports[itf - 1].did_break = false;
        break;
    case 0xFE:
        *data = (com_ports[itf - 1].cts_changed & 1) | ((com_ports[itf - 1].dsr_changed & 1) << 1) | ((com_ports[itf - 1].cts & 1) << 4) | ((com_ports[itf - 1].dsr & 1) << 5);
        
        com_ports[itf - 1].cts_changed = false;
        com_ports[itf - 1].dsr_changed = false;
        break;
    }
}

static void uart_16550_port_write(uint8_t itf, uint8_t port, uint8_t *data)
{
    // All of the UART properties are remotely configured by the PC, so there's not much to set here
    switch (port)
    {
    case 0xF8:
        if (tud_cdc_n_ready(itf))
        {
            tud_cdc_n_write(itf, data, 1);
            tud_cdc_n_write_flush(itf);

            // Throw out old data if the buffer is full while DSR is not asserted to prevent kernel deadlock
            if (!com_ports[itf - 1].dsr && !tud_cdc_n_write_available(itf))
                tud_cdc_n_write_clear(itf);
        }
        break;
    }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    if (itf == 0) return;
    itf--;

    if (com_ports[itf].cts != rts) com_ports[itf].cts_changed = true;
    if (com_ports[itf].dsr != dtr) com_ports[itf].dsr_changed = true;
    
    com_ports[itf].cts = rts;
    com_ports[itf].dsr = dtr;
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
    if (itf == 0) return;
    itf--;

    if (duration_ms >= UART_BREAK_MIN_DURATION_MS) com_ports[itf].did_break = true;
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

void uart_16550_reset(uint8_t itf)
{
    if (itf == 0) return;

    // Clear RX/TX buffers
    tud_cdc_n_write_clear(itf);
    tud_cdc_n_read_flush(itf);
    uint8_t state = tud_cdc_n_get_line_state(itf);

    itf--;
    com_ports[itf].rx_byte = 0;

    // Reset line state
    com_ports[itf].dsr = state & 1; // DTR
    com_ports[itf].cts = state & 2; // RTS
    com_ports[itf].dsr_changed = false;
    com_ports[itf].cts_changed = false;
    com_ports[itf].did_break = false;
}

static void powerup(void)
{
    uart_16550_reset(UART_1_ITF);
    uart_16550_reset(UART_2_ITF);
}

static void init(void)
{
    powerup();
}

MODXO_TASK uart_16550_hdlr = {
    .powerup = powerup,
    .init = init,
};
