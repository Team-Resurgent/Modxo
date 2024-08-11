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
#include "hardware/dma.h"
#include "hardware/spi.h"
#include "hardware/structs/bus_ctrl.h"

#include "../lpc/lpc_interface.h"
#include "SPI_PORT.h"
#include "tusb.h"

#include "SPI_PORT.h"

static struct
{
    uint baudrate;
    spi_inst_t *spi_int;
    uint8_t miso;
    uint cs_pin;
} spi_regs;

static void send_byte(uint8_t *data)
{
    if (spi_write_read_blocking(spi_regs.spi_int, data, &spi_regs.miso, 1) != 1)
        printf("\n\tSPI ERROR\n");
}

static void select_device()
{
    asm volatile("nop \n nop \n nop"); // Delays
    gpio_put(spi_regs.cs_pin, 0);      // Active low
    asm volatile("nop \n nop \n nop"); // Delays
}

static void deselect_device()
{
    asm volatile("nop \n nop \n nop"); // Delays
    gpio_put(spi_regs.cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // Delays
}

static void write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case 0xC0DE:
        // Write byte
        // select_device();
        send_byte(data);
        // deselect_device();
        break;

    case 0xC0DF:
        // TODO: Change baudrate
        break;
    }
}

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case 0xC0DE:
        // Read byte
        *data = spi_regs.miso;
        break;
    case 0xC0DF:
        // Return baudrate
        break;
    }
}

void spi_port_init(spi_inst_t *spi, uint baudrate, uint rx_pin, uint sck_pin, uint tx_pin, uint cs_pin)
{
    spi_regs.baudrate = baudrate;
    spi_regs.spi_int = spi;
    spi_init(spi, baudrate);

    if (rx_pin < 32)
        gpio_set_function(rx_pin, GPIO_FUNC_SPI);

    if (tx_pin < 32)
        gpio_set_function(tx_pin, GPIO_FUNC_SPI);

    if (sck_pin < 32)
        gpio_set_function(sck_pin, GPIO_FUNC_SPI);

    if (cs_pin < 32)
        gpio_set_function(cs_pin, GPIO_FUNC_SPI);
    // spi_regs.cs_pin = cs_pin;
    // gpio_init(cs_pin);
    // gpio_put(cs_pin, 1);
    // gpio_set_dir(cs_pin, GPIO_OUT);
    lpc_interface_add_io_handler(0xC0DE, 0xFFFE, read_handler, write_handler);
}