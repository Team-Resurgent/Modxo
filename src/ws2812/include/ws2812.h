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
    PIXEL_FORMAT_GRB,
    PIXEL_FORMAT_RGB,
    PIXEL_FORMAT_BRG,
    PIXEL_FORMAT_RBG,
    PIXEL_FORMAT_BGR,
    PIXEL_FORMAT_GBR,
} PIXEL_FORMAT_TYPE;


extern MODXO_TASK ws2812_hdlr;
extern nvm_register_t ws2812_nvm;
