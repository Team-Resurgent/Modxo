/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#ifndef _LPC_INTERFACE_H_
#define _LPC_INTERFACE_H_

#include "hardware/pio.h"
#include <modxo.h>

typedef enum
{
    LPC_OP_IO_READ = 0,
    LPC_OP_IO_WRITE = 1,
    LPC_OP_MEM_WRITE = 2,
    LPC_OP_MEM_READ = 3,
    LPC_OP_TOTAL = 4
} LPC_OP_TYPE;

typedef void (*lpc_io_handler_cback)(uint16_t address, uint8_t *data);
typedef void (*lpc_mem_handler_cback)(uint32_t address, uint8_t *data);

void lpc_interface_disable_onboard_flash(bool disable);
void lpc_interface_start_sm(void);

// I/O Interface functions
int lpc_interface_io_add_handler(uint16_t addr_start, uint16_t addr_end, lpc_io_handler_cback read_cback, lpc_io_handler_cback write_cback);
bool lpc_interface_io_set_addr(unsigned int hdlr_idx, uint16_t addr_start, uint16_t addr_end);

// Memory interface functions
void lpc_interface_mem_global_read_handler(uint8_t * read_buffer, uint32_t read_mask);
void lpc_interface_mem_global_write_handler(uint8_t * write_buffer, uint32_t write_mask);

// Map a buffer to one of 16 possible 1MB memory chunks at the one of the following paths: 0xFFX00000
// Buffer should be of size N^2 and the mask should be N^2 - 1, for example an 8KB (2^13) buffer would have the mask 8192 - 1 = 0x1FFF
bool lpc_interface_mem_set_read_handler(uint32_t address, uint8_t * read_buffer, uint32_t read_mask);
bool lpc_interface_mem_set_write_handler(uint32_t address, uint8_t * write_buffer, uint32_t write_mask);

extern MODXO_TASK lpc_interface_hdlr;
#endif