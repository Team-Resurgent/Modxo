/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once

#include "modxo_queue.h"

#include <modxo_pinout.h>

// LED
#define MODXO_REGISTER_LED_COMMAND 0xA2
void led_command_write(uint16_t address, uint8_t *data);
void led_command_read(uint16_t address, uint8_t *data);

#define MODXO_REGISTER_LED_DATA 0xA3
void led_data_write(uint16_t address, uint8_t *data);

// LPC47M152 Emulation
#if LPC47M152_SYSOPT
    #define LPC47M152_DEFAULT_CONFIG_ADDR 0x004E
#else
    #define LPC47M152_DEFAULT_CONFIG_ADDR 0x002E
#endif
void lpc47m152_config_read(uint16_t address, uint8_t * data);
void lpc47m152_config_write(uint16_t address, uint8_t * data);

#define LPC47M152_DEFAULT_DATA_ADDR (LPC47M152_DEFAULT_CONFIG_ADDR + 1)
void lpc47m152_data_read(uint16_t address, uint8_t * data);
void lpc47m152_data_write(uint16_t address, uint8_t * data);

// 16550 UART Emulation
#define UART_1_ADDR_START   0x3F8
#define UART_1_ADDR_END     0x3FF
void uart_16550_com1_read(uint16_t address, uint8_t * data);
void uart_16550_com1_write(uint16_t address, uint8_t * data);

#define UART_2_ADDR_START   0x2F8
#define UART_2_ADDR_END     0x2FF
void uart_16550_com2_read(uint16_t address, uint8_t * data);
void uart_16550_com2_write(uint16_t address, uint8_t * data);



// NVM Config
#define MODXO_REGISTER_NVM_CONFIG_IDX 0xDEA4
void nvm_config_idx_read(uint16_t address, uint8_t *data);
void nvm_config_idx_write(uint16_t address, uint8_t *data);

#define MODXO_REGISTER_NVM_CONFIG_VAL 0xDEA5
void nvm_config_val_read(uint16_t address, uint8_t *data);
void nvm_config_val_write(uint16_t address, uint8_t *data);

// Data Store
#define MODXO_REGISTER_DATA_STORE_IDX 0xDEA6
void data_store_idx_read(uint16_t address, uint8_t * data);
void data_store_idx_write(uint16_t address, uint8_t * data);

#define MODXO_REGISTER_DATA_STORE_VAL 0xDEA7
void data_store_val_read(uint16_t address, uint8_t * data);
void data_store_val_write(uint16_t address, uint8_t * data);

// LCD
#define MODXO_REGISTER_LCD_COMMAND 0xDEA8
void lcd_command_write(uint16_t address, uint8_t *data);

#define MODXO_REGISTER_LCD_DATA 0xDEA9
void lcd_data_write(uint16_t address, uint8_t *data);

// Flashrom
#define MODXO_REGISTER_BANKING   0xDEAA
#define MODXO_REGISTER_SIZE      0xDEAB
#define MODXO_REGISTER_MEM_ERASE 0xDEAC
#define MODXO_REGISTER_MEM_FLUSH 0xDEAE
void flashrom_read(uint16_t address, uint8_t *data);
void flashrom_write(uint16_t address, uint8_t *data);

// ModXo/Pico ID
#define MODXO_REGISTER_CHIP_ID 0xDEAD
void modxo_chip_id_read(uint16_t address, uint8_t * data);

#define MODXO_REGISTER_VARIANT_ID 0xDEAF
void modxo_variant_id_read(uint16_t address, uint8_t * data);

typedef enum
{
    MODXO_VARIANT_OFFICIAL_PICO,
    MODXO_VARIANT_OFFICIAL_PICO2,
    MODXO_VARIANT_RP2040_ZERO_TINY,
    MODXO_VARIANT_YD_RP2040,
    MODXO_VARIANT_XIAO_RP2040,
    MODXO_VARIANT_ULTRA,
} MODXO_VARIANT_TYPE;

typedef struct
{
    bool iscmd;
    union
    {
        uint32_t raw;
        uint8_t data;
    };
} MODXO_QUEUE_ITEM_T;

