#pragma once

#include <pico/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

void expansion_i2c_init();
int expansion_i2c_read_timeout_us(uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us);
int expansion_i2c_write_timeout_us(uint8_t addr, uint8_t *src, size_t len, bool nostop, uint timeout_us);