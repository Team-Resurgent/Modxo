/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "hardware/pio.h"

#if PICO_RP2350
	#define SYS_FREQ_DEFAULT (150 * 1000)
	#define SYS_FREQ_IN_KHZ (266 * 1000)
#else
	#define SYS_FREQ_DEFAULT (133 * 1000)
	#define SYS_FREQ_IN_KHZ (266 * 1000)
#endif


void modxo_reset(void);
void modxo_init(void);
void modxo_poll_core1(void);
void modxo_poll_core0(void);
void modxo_lpc_reset_off(void);
void modxo_lpc_reset_on(void);
void modxo_low_power_mode(void);
void modxo_enable_lpm(bool enable);

