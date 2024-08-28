/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

typedef enum
{ 
	LedColorOff = 0,
	LedColorRed = 1,
	LedColorGgreen = 2,
	LedColorAmber = 3,
	LedColorBlue = 4,
	LedColorPurple = 5,
	LedColorTeal = 6,
	LedColorWhite = 7
} LedColorEnum;

void ws2812_init();
void ws2812_poll();
void ws2812_set_color(uint8_t color);
