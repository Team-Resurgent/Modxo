#pragma once

#include <stdint.h>

void legacy_display_command(uint32_t cmd);
void legacy_display_data(uint8_t *data);
void legacy_display_poll();
void legacy_display_set_spi();
void legacy_display_set_i2c(uint8_t i2c_address);
void legacy_display_init();