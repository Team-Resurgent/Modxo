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

#define UART_FLUSH_INTERVAL (10 * 1000) // In microseconds
#define UART_BREAK_MIN_DURATION_MS 250
#define UART_ADDR_MASK      0xFF00
#define UART_1_ITF          1
#define UART_2_ITF          2

typedef struct {
    uint64_t tx_flush_last;
    uint32_t tx_bytes_total;
    uint32_t rx_bytes_total;
    bool cts, cts_changed; // This is controlled by RTS
    bool dsr, dsr_changed; // This is controlled by DTR
    bool did_break;
    bool divisor_latch;
    uint8_t rx_byte;
} UART_16550_PORT;

static UART_16550_PORT coms[2];
static UART_16550_PORT * itf_coms[3] = {NULL, coms, coms + 1};

static void uart_16550_port_read(uint8_t itf, uint8_t port, uint8_t *data)
{
    UART_16550_PORT * com = itf_coms[itf];
    cdc_line_coding_t coding;
    switch (port)
    {
    case 0xF8:
        if (com->divisor_latch)
        {
            // Divisor LSB
            tud_cdc_n_get_line_coding(itf, &coding);
            *data = (uint8_t)(115200 / coding.bit_rate);
        }
        else
        {
            // Attemt to read a byte, if this fails, then rx_byte will be unchanged
            com->rx_bytes_total += tud_cdc_n_read(itf, &com->rx_byte, 1);
            *data = com->rx_byte;
        }
        break;
    case 0xF9:
        if (com->divisor_latch)
        {
            // Divisor MSB
            tud_cdc_n_get_line_coding(itf, &coding);
            *data = (uint8_t)((115200 / coding.bit_rate) >> 8);
        }
        break;
    case 0xFB:
        tud_cdc_n_get_line_coding(itf, &coding);
        *data = ((coding.data_bits - 5) & 3) | (coding.stop_bits ? 4 : 0) | (coding.parity ? 8 : 0) | (((coding.parity - 1) & 3) << 4);
        break;
    case 0xFD:
        *data = (tud_cdc_n_available(itf) ? 0x01 : 0x00) | (com->did_break ? 0x10 : 0x00) | (tud_cdc_n_write_available(itf) ? 0x20 : 0x00);
        
        com->did_break = false;
        break;
    case 0xFE:
        *data = (com->cts_changed & 1) | ((com->dsr_changed & 1) << 1) | ((com->cts & 1) << 4) | ((com->dsr & 1) << 5);
        
        com->cts_changed = false;
        com->dsr_changed = false;
        break;
    }
}

static void uart_16550_port_write(uint8_t itf, uint8_t port, uint8_t *data)
{
    // All of the UART properties are remotely configured by the PC, so there's not much to set here
    UART_16550_PORT * com = itf_coms[itf];
    switch (port)
    {
    case 0xF8:
        if (!com->divisor_latch && tud_cdc_n_ready(itf) && tud_cdc_n_connected(itf))
        {
            com->tx_bytes_total += tud_cdc_n_write(itf, data, 1);
            tud_cdc_n_write_flush(itf);
        }
        break;
    case 0xFB:
        com->divisor_latch = !!(*data & 0x80);
        break;
    }
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts)
{
    UART_16550_PORT * com = itf_coms[itf];
    if (!com) return;

    if (com->cts != rts) com->cts_changed = true;
    if (com->dsr != dtr) com->dsr_changed = true;
    
    com->cts = rts;
    com->dsr = dtr;
}

void tud_cdc_send_break_cb(uint8_t itf, uint16_t duration_ms)
{
    UART_16550_PORT * com = itf_coms[itf];
    if (!com) return;

    if (duration_ms >= UART_BREAK_MIN_DURATION_MS) com->did_break = true;
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
    UART_16550_PORT * com = itf_coms[itf];
    if (!com) return;

    // Clear RX/TX buffers
    tud_cdc_n_write_clear(itf);
    tud_cdc_n_read_flush(itf);
    uint8_t state = tud_cdc_n_get_line_state(itf);

    itf--;
    com->rx_byte = 0;
    com->divisor_latch = false;

    // Reset line state
    com->dsr = state & 1; // DTR
    com->cts = state & 2; // RTS
    com->dsr_changed = false;
    com->cts_changed = false;
    com->did_break = false;
    com->tx_flush_last = 0;
    com->tx_bytes_total = 0;
    com->rx_bytes_total = 0;
}

static void poll_itf(uint8_t itf)
{
    if (tud_cdc_n_ready(itf) && tud_cdc_n_connected(itf))
    {
        uint64_t now = time_us_64();
        UART_16550_PORT * com = itf_coms[itf];
        if (com && now > com->tx_flush_last + UART_FLUSH_INTERVAL)
        {
            // Periodically flush data so as not to overload the USB interface
            uint32_t flushed_bytes = tud_cdc_n_write_flush(itf);
            com->tx_bytes_total += flushed_bytes;
            com->tx_flush_last = now;

#ifdef COM_LOGGING
            {
                uint32_t tx_avail = tud_cdc_n_write_available(itf);
                uint32_t rx_avail = tud_cdc_n_available(itf);

                printf("COM%u_LOG (%u TX TOTAL, %02u TX FREE | %u RX TOTAL, %02u RX PENDING | TIME %016llX): ",
                    itf,
                    com->tx_bytes_total, tx_avail,
                    com->rx_bytes_total, rx_avail,
                    now
                );

                if (flushed_bytes)
                    printf("Flushed %02u bytes", flushed_bytes);
                else
                    printf("Nothing to flush");

                printf("\n");
            }
#endif
        }
    }
    else
    {
        // Discard data when no terminal is connected to recieve it
        tud_cdc_n_write_clear(itf);
    }
}

static void poll(void)
{
    poll_itf(UART_1_ITF);
    poll_itf(UART_2_ITF);
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
    .core1_poll = poll,
};
