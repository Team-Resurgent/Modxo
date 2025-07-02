/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2025, QuantX <Sam Deutsch>
*/
#include <stddef.h>
#include <modxo/lpc_interface.h>

typedef struct
{
    uint32_t base_address;
    uint32_t mask;
    lpc_mem_read_t read_cback;
    lpc_mem_write_t write_cback;
} lpc_mem_handler;

// Give something for empty lpc_mem_handler's to point to
static uint8_t mem_read_dummy = 0xFF, mem_write_dummy;

// Decode one of 16 possible memory handlers using the following 'X' nibble: 0xFFX00000
#define MEM_HANDLER_COUNT 16
static lpc_mem_handler mem_hdlr_table[MEM_HANDLER_COUNT];

static inline lpc_mem_handler * mem_get_hdlr(uint32_t address)
{
    return mem_hdlr_table + ((address >> 20) & 0xF);
}

uint8_t mem_read_hdlr(uint32_t address)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    return hdlr->read_cback(address);
}

void mem_write_hdlr(uint32_t address, uint8_t data)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    hdlr->write_cback(address, data);
}

bool lpc_interface_mem_subscriber(uint32_t base_address, size_t mem_size, lpc_mem_read_t read_cback, lpc_mem_write_t write_cback)
{
    if ((base_address & 0xFF0FFFFF) != 0xFF000000 || mem_size > 0x100000) return false;

    lpc_mem_handler * hdlr = mem_get_hdlr(base_address);
    hdlr->read_cback = read_cback;
    hdlr->write_cback = write_cback;
    hdlr->mask = mem_size - 1;
    return true;
}
