/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "config_nvm.h"

void config_set_reg_sel( NVM_REGISTER_SEL reg);
NVM_REGISTER_SEL config_get_reg_sel(void);
void config_set_value(uint8_t value);
uint8_t config_get_value(void);
void config_nvm_reset(void);
void config_nvm_init(void);