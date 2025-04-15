/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx
*/
#pragma once

#include <stdint.h>

typedef struct
{
  void* data;
  const void* default_value;
  uint8_t size;
}nvm_register_t;

extern nvm_register_t* nvm_registers[];
extern uint8_t nvm_registers_count;

void config_save_parameters(void);
void config_retrieve_parameters(void);
void* config_get_current(void);

/*
void config_set_reg_sel( NVM_REGISTER_SEL reg);
NVM_REGISTER_SEL config_get_reg_sel(void);
*/

void config_set_value(uint8_t value);
uint8_t config_get_value(void);


void config_poll(void);