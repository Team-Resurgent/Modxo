/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "modxo_queue.h"

#include <modxo_pinout.h>

#define MODXO_REGISTER_NVM_CONFIG_SEL 0xDEA4
#define MODXO_REGISTER_NVM_CONFIG_VAL 0xDEA5
#define MODXO_REGISTER_VOLATILE_CONFIG_SEL 0xDEA6
#define MODXO_REGISTER_VOLATILE_CONFIG_VAL 0xDEA7
#define MODXO_REGISTER_CHIP_ID 0xDEAD
#define MODXO_REGISTER_VARIANT_ID 0xDEAF

#define MODXO_REGISTER_LED_COMMAND 0xA2
#define MODXO_REGISTER_LED_DATA 0xA3



typedef enum
{
    MODXO_VARIANT_OFFICIAL_PICO,
    MODXO_VARIANT_OFFICIAL_PICO2,
    MODXO_VARIANT_RP2040_ZERO_TINY,
    MODXO_VARIANT_YD_RP2040,
    MODXO_VARIANT_XIAO_RP2040,
    MODXO_VARIANT_ULTRA,
} MODXO_VARIANT_TYPE;

#pragma pack(push, 1)
typedef struct
{
    bool iscmd;
    union
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
    } data;
} MODXO_QUEUE_ITEM_T;
#pragma pack(pop)

