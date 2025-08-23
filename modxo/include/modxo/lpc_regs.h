/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct{
    uint8_t type;
    uint8_t param;
    uint8_t startIndex;
    uint8_t endIndex;
}Effect_t;

typedef struct REG_S{
    void* data;
    uint32_t length;
    uint32_t cwrite;
    uint32_t cread;
    uint8_t* write_buffer;
    uint8_t* read_buffer;
    uint8_t* value;
}REG_T;

typedef struct{
    REG_T reg;
    Effect_t write_buffer;
    Effect_t read_buffer;
}REGEffect_t;

typedef struct{
    REG_T reg;
    uint8_t write_buffer[4];
    uint8_t read_buffer[4];
}REG32bit_t;

typedef struct{
    REG_T reg;
    uint8_t write_buffer[3];
    uint8_t read_buffer[3];
}REG24bit_t;

typedef struct{
    REG_T reg;
    uint8_t write_buffer[2];
    uint8_t read_buffer[2];
}REG16bit_t;

#define LPC_REG_INIT(r,v) (r.reg.length = sizeof(r.write_buffer),r.reg.write_buffer = (uint8_t*)&r.write_buffer,r.reg.write_buffer = (uint8_t*)&r.write_buffer,r.reg.value = (uint8_t*)&v)
#define LPC_REG_READ(r,v) v = lpc_reg_read(&r.reg);
#define LPC_REG_WRITE(r,v) lpc_reg_write(&r.reg, v);
#define LPC_REG_RESET(r) lpc_reg_reset(&r.reg);

void lpc_reg_write(REG_T* reg, uint8_t data);
uint8_t lpc_reg_read(REG_T* reg);
void lpc_reg_reset(REG_T* reg);