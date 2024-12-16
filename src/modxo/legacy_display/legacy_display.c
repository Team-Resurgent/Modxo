/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx

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
#include "legacy_display.h"

#include "../modxo_queue.h"
#include "../modxo_ports.h"

#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include "../config/config_nvm.h"

#include <modxo_pinout.h>

#define LCD_QUEUE_BUFFER_LEN 1024
#define LCD_TIMEOUT_US 100

static struct
{
    MODXO_QUEUE_ITEM_T buffer[LCD_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;
    bool is_spi;
    int8_t spi_device;
    uint8_t i2c_address;
    bool has_i2c_prefix;
    uint8_t i2c_prefix;
} private_data;

void legacy_display_command(uint32_t raw)
{
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.raw = raw;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

void legacy_display_data(uint8_t *data)
{
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = false;
    _item.data = *data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

void legacy_display_poll()
{
    MODXO_QUEUE_ITEM_T _item;
    if (modxo_queue_remove(&private_data.queue, &_item))
    {
        if (_item.iscmd)
        {
            MODXO_LCD_CMD rx_cmd;
            rx_cmd.raw = _item.raw;

            switch (rx_cmd.cmd)
            {
            case MODXO_LCD_SET_SPI:
                legacy_display_set_spi(rx_cmd.param1);
                break;
            case MODXO_LCD_SET_I2C:
                legacy_display_set_i2c(rx_cmd.param1);
                break;
            case MODXO_LCD_REMOVE_I2C_PREFIX:
                legacy_display_remove_i2c_prefix();
                break;
            case MODXO_LCD_SET_I2C_PREFIX:
                legacy_display_set_i2c_prefix(rx_cmd.param1);
                break;
            default:
                break;
            }
        }
        else
        {
            if (private_data.is_spi)
            {
                uint csPin = private_data.spi_device == 0 ? LCD_PORT_SPI_CSN1 : LCD_PORT_SPI_CSN2;
                gpio_put(csPin, 0);
                spi_write_blocking(LCD_PORT_SPI_INST, &_item.data, 1);
                gpio_put(csPin, 1);
                return;
            }
            if (private_data.has_i2c_prefix)
            {
                char tempBuffer[2];
                tempBuffer[0] = private_data.i2c_prefix;
                tempBuffer[1] = _item.data;
                i2c_write_timeout_us(LCD_PORT_I2C_INST, private_data.i2c_address, tempBuffer, 2, false, LCD_TIMEOUT_US);
                return;
            }
            i2c_write_timeout_us(LCD_PORT_I2C_INST, private_data.i2c_address, &_item.data, 1, false, LCD_TIMEOUT_US);
        }
    }
}

void legacy_display_set_spi(uint8_t device)
{
    private_data.is_spi = true;
    private_data.spi_device = device;
    spi_init(LCD_PORT_SPI_INST, 100 * 1000);
    spi_set_format(LCD_PORT_SPI_INST, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    gpio_set_function(LCD_PORT_SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_PORT_SPI_MOSI, GPIO_FUNC_SPI);

    uint csPin = private_data.spi_device == 0 ? LCD_PORT_SPI_CSN1 : LCD_PORT_SPI_CSN2;
    gpio_init(csPin);
    gpio_put(csPin, 1);
    gpio_set_dir(csPin, GPIO_OUT);
}

void legacy_display_set_i2c(uint8_t i2c_address)
{
    private_data.is_spi = false;
    private_data.i2c_address = i2c_address;
    i2c_init(LCD_PORT_I2C_INST, 100 * 1000);
    gpio_set_function(LCD_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(LCD_PORT_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_PORT_I2C_SDA);
    gpio_pull_up(LCD_PORT_I2C_SCL);
}

void legacy_display_remove_i2c_prefix()
{
    private_data.has_i2c_prefix = false;
}

void legacy_display_set_i2c_prefix(uint8_t prefix)
{
    private_data.has_i2c_prefix = true;
    private_data.i2c_prefix = prefix;
}

void legacy_display_init()
{
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), LCD_QUEUE_BUFFER_LEN);
    legacy_display_set_spi(0);
}