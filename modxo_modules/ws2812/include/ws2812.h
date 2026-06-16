/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <modxo.h>
#include <modxo/config_nvm.h>
typedef enum
{ 
	LedColorOff = 0,
	LedColorRed = 1,
	LedColorGreen = 2,
	LedColorAmber = 3,
	LedColorBlue = 4,
	LedColorPurple = 5,
	LedColorTeal = 6,
	LedColorWhite = 7
} LedColorEnum;


typedef enum
{
    PIXEL_FORMAT_GRB = 0,
    PIXEL_FORMAT_RGB = 1,
    PIXEL_FORMAT_BRG = 2,
    PIXEL_FORMAT_RBG = 3,
    PIXEL_FORMAT_BGR = 4,
    PIXEL_FORMAT_GBR = 5,
	PIXEL_FORMAT_INVALID = 0xFF
} PIXEL_FORMAT_TYPE;


extern MODXO_TASK ws2812_hdlr;
extern nvm_register_t ws2812_nvm;
