/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include <stdint.h>
#include "../ws2812/ws2812.h"
#include "../display_interface/display_interface.h"

typedef struct{
    uint8_t first_bootbank_size:2;
    uint8_t enable_superio_sp:1;

    struct{
      DISPLAY_INTERFACE interface;
      uint8_t addr1;
      uint8_t addr2;
    }display_config;

    PIXEL_FORMAT_TYPE led_pf;

}MODXO_CONFIG;

typedef enum {
  NVM_REGISTER_NONE               = 0,
  NVM_REGISTER_DISPLAY_INTERFACE  = 1, //0 = spi 1 = i2c
  NVM_REGISTER_DISPLAY1_ADDRESS   = 2, 
  NVM_REGISTER_DISPLAY2_ADDRESS   = 3, 
  NVM_REGISTER_ENABLE_SUPERIO_SP  = 4,
  NVM_REGISTER_BOOT_BANK_SIZE     = 5,
  NVM_REGISTER_LED_PIXEL_FORMAT   = 6,
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