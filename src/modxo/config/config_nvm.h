/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx
*/
#pragma once

#include <stdint.h>
#include "../ws2812/ws2812.h"

typedef struct{
    PIXEL_FORMAT_TYPE rgb_status_pf;
    PIXEL_FORMAT_TYPE rgb_strip_pf[4];

}MODXO_CONFIG;

typedef enum {
  NVM_REGISTER_NONE               =  0,
  NVM_REGISTER_RGB_STATUS_PF      =  1,
  NVM_REGISTER_RGB_STRIP1_PF      =  2,
  NVM_REGISTER_RGB_STRIP2_PF      =  3,
  NVM_REGISTER_RGB_STRIP3_PF      =  4,
  NVM_REGISTER_RGB_STRIP4_PF      =  5,
} NVM_REGISTER_SEL;

extern MODXO_CONFIG config;

void config_save_parameters(void);
void config_retrieve_parameters(void);
MODXO_CONFIG* config_get_current(void);

void config_set_reg_sel( NVM_REGISTER_SEL reg);
NVM_REGISTER_SEL config_get_reg_sel(void);
void config_set_value(uint8_t value);
uint8_t config_get_value(void);
void config_poll(void);

void config_nvm_init(void);