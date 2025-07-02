/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#ifndef _LPC_INTERFACE_H_
#define _LPC_INTERFACE_H_
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <modxo.h>

typedef uint8_t (*lpc_mem_read_t )  (uint32_t address);
typedef uint8_t (*lpc_io_read_t  )  (uint16_t address);
typedef void    (*lpc_mem_write_t)  (uint32_t address, uint8_t data);
typedef void    (*lpc_io_write_t )  (uint16_t address, uint8_t data);

typedef bool    (*lpc_sub_io)   (uint16_t port_base, size_t total_ports, lpc_io_read_t read_cback, lpc_io_write_t write_cback);
typedef bool    (*lpc_sub_mem) (uint32_t port_base, size_t mem_size, lpc_mem_read_t read_cback, lpc_mem_write_t write_cback);

typedef struct
{
    lpc_mem_read_t mem_read;
    lpc_io_read_t  io_read;
    lpc_mem_write_t mem_write;
    lpc_io_write_t io_write;
    
    lpc_sub_io io_subscriber;
    lpc_sub_mem mem_subscriber;
} lpc_op_mapper_t;

extern lpc_op_mapper_t lpc_op_mapper;//This will link only one mapper to all LPC operations
bool lpc_register_io_handler ( uint16_t port_base   , size_t total_regs, lpc_io_read_t  read_cback, lpc_io_write_t  write_cback);
bool lpc_register_mem_handler( uint32_t address_base, size_t mem_size  , lpc_mem_read_t read_cback, lpc_mem_write_t write_cback);

void lpc_interface_disable_onboard_flash(bool disable);
void lpc_interface_start_sm(void);

extern MODXO_TASK lpc_interface_hdlr;
#endif