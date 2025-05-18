/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, QuantX <Sam Deutsch>
*/
#pragma once

#define MSC_DEFAULT_BLOCK_SIZE 512

#include <stdint.h>
#include <stdbool.h>

// IMPORTANT README: Apparently Windows won't mount MSC's with a combined (block_count * block_size < 8Kbyte)

typedef int32_t (*msc_handler_cback)(uint32_t block, uint32_t offset, uint8_t *data, uint32_t size);

bool msc_interface_add_handler(uint32_t block_count, uint32_t block_size, const char * product_str, msc_handler_cback read_cback, msc_handler_cback write_cback);