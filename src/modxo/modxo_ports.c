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
#include <pico/platform.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/structs/bus_ctrl.h>
#include <hardware/flash.h>

#include "lpc/lpc_interface.h"

#include "modxo_ports.h"
#include "flashrom/flashrom.h" // Delete
#include "pico/multicore.h"
#include "superio/SPI_PORT.h"

#define STORAGE_CMD_TOTAL_BYTES 64

static MODXO_TD_DRIVER_T *text_lcd_driver;

enum
{
    WAIT_PASSWORD,
    SECTOR_NUMBER,
    SECTOR_ERASING
} erase_password;

static int _erase_sector_number = -1;
static int _program_sector_number = -1;
uint8_t password_index = 0;
uint8_t flash_size = 0;

char password_sequence[] = "DIE";

static struct
{
    union
    {
        struct
        {
            uint8_t text_lcd : 1;  // If text lcd is handled
            uint8_t graph_lcd : 1; // If graphical lcd is handled
            uint8_t user_sp : 1;   // If there is a serial port configured for printing messages
        } bits;
        uint8_t byte;
    };
} modxo_features = {
    .byte = 0};

static uint8_t cmd_byte_idx; // Index Byte Read
MODXO_TD_CMD command_buffer;

static void write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {

    /*TEXT LCD Ports*/
    case MODXO_REGISTER_LCD_DATA: // Text LCD Character Port
        if (text_lcd_driver && text_lcd_driver->data)
        {
            text_lcd_driver->data(data);
        }
        cmd_byte_idx = 0;
        break;
    case MODXO_REGISTER_LCD_COMMAND: // Text LCD Command Port (Home, Clear, Backlight, Contrast etc...)

        if (text_lcd_driver && text_lcd_driver->command)
        {
            command_buffer.bytes[cmd_byte_idx] = *data;

            if (cmd_byte_idx == 0)
            {

                switch (command_buffer.cmd)
                {
                case MODXO_TD_SET_CURSOR_POSITION:
                case MODXO_TD_SET_CONTRAST_BACKLIGHT:
                case MODXO_TD_SEND_CUSTOM_CHAR_DATA:
                    cmd_byte_idx++;
                default:
                    text_lcd_driver->command(command_buffer.raw);
                }
            }
            else
            {
                cmd_byte_idx++;
                switch (command_buffer.cmd)
                {
                case MODXO_TD_SET_CURSOR_POSITION:
                    if (cmd_byte_idx == 3)
                    {
                        text_lcd_driver->command(command_buffer.raw);
                        cmd_byte_idx = 0;
                    }
                    break;

                case MODXO_TD_SET_CONTRAST_BACKLIGHT:
                case MODXO_TD_SEND_CUSTOM_CHAR_DATA:
                    if (cmd_byte_idx == 2)
                    {
                        text_lcd_driver->command(command_buffer.raw);
                        cmd_byte_idx = 0;
                    }
                    break;
                default:
                    cmd_byte_idx = 0;
                }
            }
        }
        break;

    // Memory Management Controller Registers
    case MODXO_REGISTER_BANKING:
        flashrom_set_mmc(*data);
        break;

    // Sector Erase
    case MODXO_REGISTER_MEM_ERASE:
        if (password_sequence[password_index] == 0)
        {
            _erase_sector_number = *data;
            password_index = 0;
            __sev();
        }
        else if (password_sequence[password_index] == *data)
        {
            password_index++;
        }
        else
        {
            password_index = 0;
        }

        break;

    // Sector Flush
    case MODXO_REGISTER_MEM_FLUSH:
        _program_sector_number = *data;
        __sev();
        break;

    }
}

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    // Memory Management Controller Registers
    case MODXO_REGISTER_BANKING:
        *data = flashrom_get_mmc();
        break;

    case MODXO_REGISTER_SIZE:
        if (flash_size == 0)
        {
            uint8_t txbuf[STORAGE_CMD_TOTAL_BYTES] = {0x9f};
            uint8_t rxbuf[STORAGE_CMD_TOTAL_BYTES] = {0};
            flash_do_cmd(txbuf, rxbuf, STORAGE_CMD_TOTAL_BYTES);
            flash_size = 1 << (rxbuf[3] - 20);
        }
        *data = flash_size;

        break;
    case  MODXO_REGISTER_MEM_ERASE:
        // Is Erasing
        *data = _erase_sector_number < 0 ? false : true;
        break;
    case MODXO_REGISTER_CHIP_ID:
        *data = 0xAF;
        break;
    case MODXO_REGISTER_MEM_FLUSH:
        // Is Programming
        *data = _program_sector_number < 0 ? false : true;
        break;
    }
}

int modxo_ports_get_erase_sector(void)
{
    return _erase_sector_number;
}

int modxo_ports_get_program_sector(void)
{
    return _program_sector_number;
}

void modxo_ports_erase_done(void)
{
    _erase_sector_number = -1;
}

void modxo_ports_program_done(void)
{
    _program_sector_number = -1;
}

void modxo_ports_poll(void)
{ // Core 1
    uint32_t val;
    if (modxo_ports_get_erase_sector() >= 0)
    {
        flashrom_erase_sector(modxo_ports_get_erase_sector());
        modxo_ports_erase_done();
    }

    if (modxo_ports_get_program_sector() >= 0)
    {
        flashrom_program_sector(modxo_ports_get_program_sector());
        modxo_ports_program_done();
    }
}

void modxo_ports_init(MODXO_TD_DRIVER_T *drv)
{
    lpc_interface_add_io_handler(MODXO_REGISTER_LCD_DATA, 0xFFF8, read_handler, write_handler);
    _program_sector_number = -1;
    _erase_sector_number = -1;
    cmd_byte_idx = 0;

    if (drv != NULL)
    {
        text_lcd_driver = drv;
    }
    else
    {
        // If no text driver was provided, SPI Passthrough will be configured by default
        // Using the same LCD Pins
        spi_port_init(SPI_DISPLAY_INSTANCE, SPI_DISPLAY_BAUDRATE,
                      SPI_DISPLAY_RX_PIN, SPI_DISPLAY_SCK_PIN,
                      SPI_DISPLAY_TX_PIN, SPI_DISPLAY_SCN_PIN);
    }
}