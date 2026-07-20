/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <modxo.h>

void serirq_trigger_irq(uint32_t irq);

extern MODXO_TASK serirq_hdlr;
