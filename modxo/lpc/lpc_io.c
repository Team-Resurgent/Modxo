/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, QuantX <Sam Deutsch>
*/
#include <modxo/lpc_interface.h>
#include <modxo/modxo_ports.h>

static void io_read_dummy(uint16_t address, uint8_t * data) { *data = 0xFF; }
static void io_write_dummy(uint16_t address, uint8_t * data) {}

typedef enum {
    LPC_IO_READ_HDLR_DUMMY = 0,

    LPC_IO_READ_HDLR_MODXO_CHIP_ID,
    LPC_IO_READ_HDLR_MODXO_VARIANT_ID,

    LPC_IO_READ_HDLR_DATA_STORE_IDX,
    LPC_IO_READ_HDLR_DATA_STORE_VAL,

    LPC_IO_READ_LPC47M152_CONFIG,
    LPC_IO_READ_LPC47M152_DATA,

    LPC_IO_READ_16550_UART_1,
    LPC_IO_READ_16550_UART_2,

    LPC_IO_READ_LED_COMMAND,

    LPC_IO_READ_NVM_CONFIG_IDX,
    LPC_IO_READ_NVM_CONFIG_VAL,

    LPC_IO_READ_FLASHROM,
} LPC_IO_READ_HDLR;

typedef enum {
    LPC_IO_WRITE_HDLR_DUMMY = 0,

    LPC_IO_WRITE_HDLR_LCD_COMMAND,
    LPC_IO_WRITE_HDLR_LCD_DATA,

    LPC_IO_WRITE_HDLR_DATA_STORE_IDX,
    LPC_IO_WRITE_HDLR_DATA_STORE_VAL,

    LPC_IO_WRITE_LPC47M152_CONFIG,
    LPC_IO_WRITE_LPC47M152_DATA,

    LPC_IO_WRITE_16550_UART_1,
    LPC_IO_WRITE_16550_UART_2,

    LPC_IO_WRITE_LED_COMMAND,
    LPC_IO_WRITE_LED_DATA,

    LPC_IO_WRITE_NVM_CONFIG_IDX,
    LPC_IO_WRITE_NVM_CONFIG_VAL,

    LPC_IO_WRITE_FLASHROM,
} LPC_IO_WRITE_HDLR;

const static uint8_t io_read_hdlr_lookup[0x10000] = {
    [MODXO_REGISTER_CHIP_ID] = LPC_IO_READ_HDLR_MODXO_CHIP_ID,
    [MODXO_REGISTER_VARIANT_ID] = LPC_IO_READ_HDLR_MODXO_VARIANT_ID,

    [MODXO_REGISTER_DATA_STORE_IDX] = LPC_IO_READ_HDLR_DATA_STORE_IDX,
    [MODXO_REGISTER_DATA_STORE_VAL] = LPC_IO_READ_HDLR_DATA_STORE_VAL,

    [LPC47M152_DEFAULT_CONFIG_ADDR] = LPC_IO_READ_LPC47M152_CONFIG,
    [LPC47M152_DEFAULT_DATA_ADDR] = LPC_IO_READ_LPC47M152_DATA,

    [UART_1_ADDR_START ... UART_1_ADDR_END] = LPC_IO_READ_16550_UART_1,
    [UART_2_ADDR_START ... UART_2_ADDR_END] = LPC_IO_READ_16550_UART_2,

    [MODXO_REGISTER_LED_COMMAND] = LPC_IO_READ_LED_COMMAND,

    [MODXO_REGISTER_NVM_CONFIG_IDX] = LPC_IO_READ_NVM_CONFIG_IDX,
    [MODXO_REGISTER_NVM_CONFIG_VAL] = LPC_IO_READ_NVM_CONFIG_VAL,

    [MODXO_REGISTER_BANKING ... MODXO_REGISTER_MEM_FLUSH] = LPC_IO_READ_FLASHROM,
};

const static uint8_t io_write_hdlr_lookup[0x10000] = {
    [MODXO_REGISTER_LCD_COMMAND] = LPC_IO_WRITE_HDLR_LCD_COMMAND,
    [MODXO_REGISTER_LCD_DATA] = LPC_IO_WRITE_HDLR_LCD_DATA,

    [MODXO_REGISTER_DATA_STORE_IDX] = LPC_IO_WRITE_HDLR_DATA_STORE_IDX,
    [MODXO_REGISTER_DATA_STORE_VAL] = LPC_IO_WRITE_HDLR_DATA_STORE_VAL,

    [LPC47M152_DEFAULT_CONFIG_ADDR] = LPC_IO_WRITE_LPC47M152_CONFIG,
    [LPC47M152_DEFAULT_DATA_ADDR] = LPC_IO_WRITE_LPC47M152_DATA,

    [UART_1_ADDR_START ... UART_1_ADDR_END] = LPC_IO_WRITE_16550_UART_1,
    [UART_2_ADDR_START ... UART_2_ADDR_END] = LPC_IO_WRITE_16550_UART_2,

    [MODXO_REGISTER_LED_COMMAND] = LPC_IO_WRITE_LED_COMMAND,
    [MODXO_REGISTER_LED_DATA] = LPC_IO_WRITE_LED_DATA,

    [MODXO_REGISTER_NVM_CONFIG_IDX] = LPC_IO_WRITE_NVM_CONFIG_IDX,
    [MODXO_REGISTER_NVM_CONFIG_VAL] = LPC_IO_WRITE_NVM_CONFIG_VAL,

    [MODXO_REGISTER_BANKING ... MODXO_REGISTER_MEM_FLUSH] = LPC_IO_WRITE_FLASHROM,
};

const static lpc_io_handler_cback io_read_hdlr_table[] = {
    [LPC_IO_READ_HDLR_DUMMY] = io_read_dummy,

    [LPC_IO_READ_HDLR_MODXO_CHIP_ID] = modxo_chip_id_read,
    [LPC_IO_READ_HDLR_MODXO_VARIANT_ID] = modxo_variant_id_read,

    [LPC_IO_READ_HDLR_DATA_STORE_IDX] = data_store_idx_read,
    [LPC_IO_READ_HDLR_DATA_STORE_VAL] = data_store_val_read,

    [LPC_IO_READ_LPC47M152_CONFIG] = lpc47m152_config_read,
    [LPC_IO_READ_LPC47M152_DATA] = lpc47m152_data_read,

    [LPC_IO_READ_16550_UART_1] = uart_16550_com1_read,
    [LPC_IO_READ_16550_UART_2] = uart_16550_com2_read,

    [LPC_IO_READ_LED_COMMAND] = led_command_read,

    [LPC_IO_READ_NVM_CONFIG_IDX] = nvm_config_idx_read,
    [LPC_IO_READ_NVM_CONFIG_VAL] = nvm_config_val_read,

    [LPC_IO_READ_FLASHROM] = flashrom_read,
};

const static lpc_io_handler_cback io_write_hdlr_table[] = {
    [LPC_IO_WRITE_HDLR_DUMMY] = io_write_dummy,

    [LPC_IO_WRITE_HDLR_LCD_COMMAND] = lcd_command_write,
    [LPC_IO_WRITE_HDLR_LCD_DATA] = lcd_data_write,

    [LPC_IO_WRITE_HDLR_DATA_STORE_IDX] = data_store_idx_write,
    [LPC_IO_WRITE_HDLR_DATA_STORE_VAL] = data_store_val_write,

    [LPC_IO_WRITE_LPC47M152_CONFIG] = lpc47m152_config_write,
    [LPC_IO_WRITE_LPC47M152_DATA] = lpc47m152_data_write,

    [LPC_IO_WRITE_16550_UART_1] = uart_16550_com1_write,
    [LPC_IO_WRITE_16550_UART_2] = uart_16550_com2_write,

    [LPC_IO_WRITE_LED_COMMAND] = led_command_write,
    [LPC_IO_WRITE_LED_DATA] = led_data_write,

    [LPC_IO_WRITE_NVM_CONFIG_IDX] = nvm_config_idx_write,
    [LPC_IO_WRITE_NVM_CONFIG_VAL] = nvm_config_val_write,

    [LPC_IO_WRITE_FLASHROM] = flashrom_write,
};

void io_read_hdlr(uint32_t address, uint8_t * data)
{
    io_read_hdlr_table[io_read_hdlr_lookup[address & 0xFFFF]](address, data);
}

void io_write_hdlr(uint32_t address, uint8_t * data)
{
    io_write_hdlr_table[io_write_hdlr_lookup[address & 0xFFFF]](address, data);
}