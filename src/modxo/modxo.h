/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "hardware/pio.h"

void modxo_reset(void);
void modxo_init(void);
void modxo_poll_core1(void);
void modxo_poll_core0(void);
void modxo_lpc_reset_off(void);
void modxo_lpc_reset_on(void);
void modxo_low_power_mode(void);

