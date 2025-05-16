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
bool lpc_interface_add_io_handler(uint16_t addr_start, uint16_t addr_end, lpc_io_handler_cback read_cback, lpc_io_handler_cback write_cback);
bool lpc_interface_add_mem_handler(uint32_t addr_start, uint32_t addr_end, lpc_mem_handler_cback read_cback, lpc_mem_handler_cback write_cback);

extern MODXO_TASK lpc_interface_hdlr;
#endif