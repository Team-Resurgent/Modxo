/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "modxo_queue.h"
#include <legacy_display.h>

#include <modxo_pinout.h>

#define MODXO_REGISTER_NVM_CONFIG_SEL 0xDEA4
#define MODXO_REGISTER_NVM_CONFIG_VAL 0xDEA5
#define MODXO_REGISTER_VOLATILE_CONFIG_SEL 0xDEA6
#define MODXO_REGISTER_VOLATILE_CONFIG_VAL 0xDEA7
#define MODXO_REGISTER_LCD_COMMAND 0xDEA8
#define MODXO_REGISTER_LCD_DATA 0xDEA9
#define MODXO_REGISTER_BANKING 0xDEAA
#define MODXO_REGISTER_SIZE 0xDEAB
#define MODXO_REGISTER_MEM_ERASE 0xDEAC
#define MODXO_REGISTER_CHIP_ID 0xDEAD
#define MODXO_REGISTER_MEM_FLUSH 0xDEAE
#define MODXO_REGISTER_VARIANT_ID 0xDEAF
#define MODXO_REGISTER_LED_COMMAND 0xA2
#define MODXO_REGISTER_LED_DATA 0xA3

#define MODXO_LCD_SET_SPI 0
#define MODXO_LCD_SET_I2C 1
#define MODXO_LCD_REMOVE_I2C_PREFIX 2
#define MODXO_LCD_SET_I2C_PREFIX 3
#define MODXO_LCD_SET_CLK 4
#define MODXO_LCD_SET_SPI_MODE 5

typedef enum
{
    MODXO_VARIANT_OFFICIAL_PICO,
    MODXO_VARIANT_OFFICIAL_PICO2,
    MODXO_VARIANT_RP2040_ZERO_TINY,
    MODXO_VARIANT_YD_RP2040,
    MODXO_VARIANT_XIAO_RP2040,
    MODXO_VARIANT_ULTRA,
} MODXO_VARIANT_TYPE;

typedef union
{
    struct
    {
        uint8_t cmd;
        uint8_t param1;
        uint8_t param2;
        uint8_t param3;
    };
    uint8_t bytes[4];
    uint32_t raw;
} MODXO_LCD_CMD;

typedef struct
{
    bool iscmd;
    union
    {
        uint32_t raw;
        uint8_t data;
    };
} MODXO_QUEUE_ITEM_T;

int modxo_ports_get_erase_sector();
int modxo_ports_get_program_sector();
void modxo_ports_erase_done(void);
void modxo_ports_program_done(void);
void modxo_ports_poll(void);
void modxo_ports_reset(void);
void modxo_ports_init(void);
