/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include "modxo_td_driver.h"
#include "hardware/i2c.h"

typedef enum
{
    HD44780_LCDTYPE_16X2 = 0,
    HD44780_LCDTYPE_16X4 = 1,
    HD44780_LCDTYPE_20X2 = 2,
    HD44780_LCDTYPE_20X4 = 3,
    HD44780_LCDTYPE_40X2 = 4,
    HD44780_LCDTYPE_40X4 = 5,
    HD44780_LCDTYPE_TOTAL_TYPES
} HD44780_LCDTYPE;

bool hd44780_i2c_init(MODXO_TD_DRIVER_T *drv, uint8_t address, HD44780_LCDTYPE lcdtype);
