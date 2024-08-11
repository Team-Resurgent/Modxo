/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "modxo_queue.h"
#include "text_display/modxo_td_driver.h"

#define MODXO_REGISTER_DATA_STORE_COMMAND 0xDEA6
#define MODXO_REGISTER_DATA_STORE_DATA 0xDEA7
#define MODXO_REGISTER_LCD_DATA 0xDEA8
#define MODXO_REGISTER_LCD_COMMAND 0xDEA9
#define MODXO_REGISTER_BANKING 0xDEAA
#define MODXO_REGISTER_SIZE 0xDEAB
#define MODXO_REGISTER_MEM_ERASE 0xDEAC
#define MODXO_REGISTER_CHIP_ID 0xDEAD
#define MODXO_REGISTER_MEM_FLUSH 0xDEAE
#define MODXO_REGISTER_LED_COMMAND 0xA2
#define MODXO_REGISTER_LED_DATA 0xA3

#define MODXO_TD_INIT 0
#define MODXO_TD_CLEAR_DISPLAY 1
#define MODXO_TD_RETURN_HOME 2
#define MODXO_TD_ENTRY_MODE_SET 3
#define MODXO_TD_DISPLAY_CONTROL 4
#define MODXO_TD_CURSOR_DISPLAY_SHIFT 5
#define MODXO_TD_SET_CURSOR_POSITION 6
#define MODXO_TD_SET_CONTRAST_BACKLIGHT 7
#define MODXO_TD_SELECT_CUSTOM_CHAR 14
#define MODXO_TD_SEND_CUSTOM_CHAR_DATA 15

typedef union
{
    struct
    {
        uint8_t param1 : 4; // Byte0 Low Nibble
        uint8_t cmd : 4;    // Byte0 High Nibble
        uint8_t param2;
        uint8_t param3;
        uint8_t param4;
    };
    uint8_t bytes[4];
    uint32_t raw;
} MODXO_TD_CMD;

typedef struct
{
    bool iscmd;
    union
    {
        uint32_t cmd;
        uint8_t data;
    };
} MODXO_QUEUE_ITEM_T;

int modxo_ports_get_erase_sector();
int modxo_ports_get_program_sector();
void modxo_ports_erase_done(void);
void modxo_ports_program_done(void);
void modxo_ports_poll(void);
void modxo_ports_init(MODXO_TD_DRIVER_T *drv);
