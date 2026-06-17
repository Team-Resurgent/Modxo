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
	LedColorOff,
	LedColorRed,
	LedColorGreen,
	LedColorAmber,
	LedColorBlue,
	LedColorPurple,
	LedColorTeal,
	LedColorWhite,
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
