/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, QuantX <Sam Deutsch>
*/
#include <stddef.h>
#include <modxo/lpc_interface.h>

typedef struct
{
    uint8_t * read_buffer;
    uint32_t read_mask;
    uint8_t * write_buffer;
    uint32_t write_mask;
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

void mem_read_hdlr(uint32_t address, uint8_t * data)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    *data = hdlr->read_buffer[address & hdlr->read_mask];
}

void mem_write_hdlr(uint32_t address, uint8_t * data)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    hdlr->write_buffer[address & hdlr->write_mask] = *data;
}

void lpc_interface_mem_global_read_handler(uint8_t * read_buffer, uint32_t read_mask)
{
    if (read_buffer != NULL)
    {
        for (unsigned int i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            mem_hdlr_table[i].read_buffer = read_buffer;
            mem_hdlr_table[i].read_mask = read_mask & 0x00FFFFFF;
        }
    }
    else
    {
        for (unsigned int i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            // Point to the dummy buffer to avoid memory errors
            mem_hdlr_table[i].read_buffer = &mem_read_dummy;
            mem_hdlr_table[i].read_mask = 0;
        }
    }
}

void lpc_interface_mem_global_write_handler(uint8_t * write_buffer, uint32_t write_mask)
{
    if (write_buffer != NULL)
    {
        for (unsigned int i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            mem_hdlr_table[i].write_buffer = write_buffer;
            mem_hdlr_table[i].write_mask = write_mask & 0x00FFFFFF;
        }
    }
    else
    {
        for (unsigned int i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            // Point to the dummy buffer to avoid memory errors
            mem_hdlr_table[i].write_buffer = &mem_write_dummy;
            mem_hdlr_table[i].write_mask = 0;
        }
    }
}

bool lpc_interface_mem_set_read_handler(uint32_t address, uint8_t * read_buffer, uint32_t read_mask)
{
    if ((address & 0xFF0FFFFF) != 0xFF000000) return false;

    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    if (read_buffer != NULL)
    {
        hdlr->read_buffer = read_buffer;
        hdlr->read_mask = read_mask & 0x00FFFFFF;
    }
    else
    {
        hdlr->read_buffer = &mem_read_dummy;
        hdlr->read_mask = 0;
    }

    return true;
}

bool lpc_interface_mem_set_write_handler(uint32_t address, uint8_t * write_buffer, uint32_t write_mask)
{
    if ((address & 0xFF0FFFFF) != 0xFF000000) return false;

    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    if (write_buffer != NULL)
    {
        hdlr->write_buffer = write_buffer;
        hdlr->write_mask = write_mask & 0x00FFFFFF;
    }
    else
    {
        hdlr->write_buffer = &mem_write_dummy;
        hdlr->write_mask = 0;
    }

    return true;
}