/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "lpc_interface.h"

typedef struct
{
    uint32_t address;
    LPC_OP_TYPE cyc_type;
    uint8_t data;
} log_entry;

void lpclog_enqueue(log_entry item);
bool lpclog_dequeue(log_entry *out);
bool lpclog_is_full(void);