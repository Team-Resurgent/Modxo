/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include <stdint.h>
#include "../ws2812/ws2812.h"

typedef struct{
    uint8_t first_bootbank_size:2;
    uint8_t enable_superio_sp:1;
    uint8_t display_port_address; // 0 = SPI  !=0 I2C address
    PIXEL_FORMAT_TYPE led_pf;
}MODXO_CONFIG;

typedef enum {
  NVM_REGISTER_NONE               = 0,
  NVM_REGISTER_LCD_TYPE           = 1,
  NVM_REGISTER_ENABLE_SUPERIO_SP  = 2,
  NVM_REGISTER_BOOT_BANK_SIZE     = 3,
  NVM_REGISTER_LED_PIXEL_FORMAT   = 4,
} NVM_REGISTER_SEL;

extern MODXO_CONFIG config;

void config_save_parameters(void);
void config_retrieve_parameters(void);
MODXO_CONFIG* config_get_current(void);

void config_set_reg_sel( NVM_REGISTER_SEL reg);
NVM_REGISTER_SEL config_get_reg_sel(void);
void config_set_value(uint8_t value);
uint8_t config_get_value(void);

void config_nvm_init(void);