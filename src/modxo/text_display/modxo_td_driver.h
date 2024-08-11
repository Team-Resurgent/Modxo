/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
typedef struct
{
    void (*command)(uint32_t cmd);
    void (*data)(uint8_t *data);
    void (*poll)();
    uint8_t rows;
    uint8_t cols;
    uint8_t status; // Busy flag, Initialized
} MODXO_TD_DRIVER_T;
