#pragma once
#include <modxo.h>
#include <stdint.h>

void legacy_display_command(uint32_t raw);
void legacy_display_data(uint8_t *data);
void legacy_display_set_clk(uint8_t clk_khz);
void legacy_display_set_spi_mode(uint8_t spi_mode);
void legacy_display_set_spi(uint8_t device);
void legacy_display_set_i2c(uint8_t i2c_address);
void legacy_display_remove_i2c_prefix();
void legacy_display_set_i2c_prefix(uint8_t prefix);
extern MODXO_TASK legacy_display_hdlr;