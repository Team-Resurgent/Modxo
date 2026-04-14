/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <expansion.h>

#include <modxo.h>
#include <modxo/modxo_queue.h>
#include <modxo/modxo_ports.h>
#include <modxo/lpc_interface.h>

#include <pico.h>
#include <string.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"
#include <modxo/config_nvm.h>

#include <modxo_pinout.h>

#define EXPANSION_QUEUE_BUFFER_LEN 1024
#define EXPANSION_MAX_BUFFER_LEN 255
#define EXPANSION_TIMEOUT_US 100000

#pragma pack(push, 1)
typedef union
{
    struct
    {
        uint8_t cmd;
        uint8_t param1;
        uint8_t param2;
        uint8_t param3;
    };
    uint8_t bytes[4];
    uint32_t raw;
} MODXO_EXPANSION_CMD;
#pragma pack(pop)

static uint8_t cmd_byte = EXPANSION_COMMAND_NONE;

static struct
{
    MODXO_QUEUE_ITEM_T buffer[EXPANSION_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;

    uint8_t i2c_address_exists_ready;
    uint8_t i2c_address_exists;

    uint8_t outgoing_index;
    uint8_t outgoing_length;
    uint8_t outgoing_buffer[EXPANSION_MAX_BUFFER_LEN];

    uint8_t incoming_values_ready;
    uint8_t incoming_index;
    uint8_t incoming_length;
    uint8_t incoming_buffer[EXPANSION_MAX_BUFFER_LEN];
} private_data;

static void does_address_exist(uint8_t i2c_address)
{
    uint8_t temp = 0;
    private_data.i2c_address_exists = (i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, i2c_address, (uint8_t*)&temp, 1, false, EXPANSION_TIMEOUT_US) >= 0) ? 1 : 0;
    private_data.i2c_address_exists_ready = 1;
}

static void send_outgoing_buffer(uint8_t i2c_address)
{
    uint8_t outgoing_length = private_data.outgoing_length;
    if (private_data.outgoing_index < outgoing_length)
    {
        return;
    }
    if (i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, i2c_address, &outgoing_length, 1, false, EXPANSION_TIMEOUT_US) == 1 && outgoing_length > 0) 
    {
        i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, i2c_address, private_data.outgoing_buffer, outgoing_length, false, EXPANSION_TIMEOUT_US);
    }
}

static void receive_incoming_buffer(uint8_t i2c_address)
{
    uint8_t incoming_length;
    if (i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, i2c_address, &incoming_length, 1, false, EXPANSION_TIMEOUT_US) == 1)
    {
        private_data.incoming_length = incoming_length;
        if (incoming_length > 0) {
            i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, i2c_address, private_data.incoming_buffer, incoming_length, false, EXPANSION_TIMEOUT_US);
        }
    }
    private_data.incoming_values_ready = 1;
}

static void expansion_poll()
{
    MODXO_QUEUE_ITEM_T _item;
    if (modxo_queue_remove(&private_data.queue, &_item))
    {
        if (_item.iscmd)
        {
            MODXO_EXPANSION_CMD rx_cmd;
            rx_cmd.raw = _item.data.raw;
            switch (rx_cmd.cmd)
            {
                case EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST:
                    does_address_exist(rx_cmd.param1 & 0x7f);
                    break;
                case EXPANSION_COMMAND_REQUEST_INCOMING_VALUES:
                    receive_incoming_buffer(rx_cmd.param1 & 0x7f);
                    break; 
                case EXPANSION_COMMAND_SEND_OUTGOING_VALUES:
                    send_outgoing_buffer(rx_cmd.param1 & 0x7f);
                    break;
            }
        }
        __sev();
    }
}

static void queue_expansion_command(uint32_t cmd, uint32_t data)
{
    if (cmd == EXPANSION_COMMAND_NONE) {
        return;
    }

    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.data.cmd = cmd;
    _item.data.param1 = data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

static void write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
        case MODXO_REGISTER_EXPANSION_DATA: 
        {
            switch (cmd_byte)
            {
                case EXPANSION_COMMAND_SET_INCOMING_INDEX:
                    private_data.incoming_index = *data;
                    break;
                case EXPANSION_COMMAND_SET_OUTGOING_LENGTH:
                    private_data.outgoing_index = 0;
                    private_data.outgoing_length = *data;
                    break;
                case EXPANSION_COMMAND_SET_OUTGOING_INDEX:
                    private_data.outgoing_index = *data;
                    break;
                case EXPANSION_COMMAND_SET_OUTGOING_VALUE:
                    if (private_data.outgoing_index < private_data.outgoing_length) 
                    {
                        private_data.outgoing_buffer[private_data.outgoing_index] = *data;
                        private_data.outgoing_index++;
                    }
                    break;
                case EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST:
                    private_data.i2c_address_exists_ready = 0;
                    queue_expansion_command(cmd_byte, *data);
                    break;
                case EXPANSION_COMMAND_REQUEST_INCOMING_VALUES:
                    private_data.incoming_values_ready = 0;
                    queue_expansion_command(cmd_byte, *data);
                    break; 
                case EXPANSION_COMMAND_SEND_OUTGOING_VALUES:
                    queue_expansion_command(cmd_byte, *data);
                    break;
            }
            break;
        }
        case MODXO_REGISTER_EXPANSION_COMMAND: 
        {
            switch (*data)
            {
                case EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST:
                    private_data.i2c_address_exists_ready = 0;
                    private_data.i2c_address_exists = 0;
                    cmd_byte = *data;
                    break;
                case EXPANSION_COMMAND_REQUEST_INCOMING_VALUES:
                    private_data.incoming_values_ready = 0;
                    private_data.incoming_index = 0;
                    cmd_byte = *data;
                    break;
                case EXPANSION_COMMAND_SET_INCOMING_INDEX:
                case EXPANSION_COMMAND_SET_OUTGOING_LENGTH:
                case EXPANSION_COMMAND_SET_OUTGOING_INDEX:
                case EXPANSION_COMMAND_SET_OUTGOING_VALUE:
                case EXPANSION_COMMAND_SEND_OUTGOING_VALUES:
                case EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXIST_READY:
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUES_READY:
                case EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXISTS:
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_LENGTH:
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUE:
                    cmd_byte = *data;
                    break;
                default:
                    cmd_byte = EXPANSION_COMMAND_NONE;
            }
            break;
        }
        break;
    }
}

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
        case MODXO_REGISTER_EXPANSION_DATA:
        {
            switch (cmd_byte)
            {
                case EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXIST_READY:
                    *data = private_data.i2c_address_exists_ready;
                    break;
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUES_READY:
                    *data = private_data.incoming_values_ready;
                    break;
                case EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXISTS:
                    *data = private_data.i2c_address_exists;
                    break;
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_LENGTH:
                    *data = private_data.incoming_length;
                    break;
                case EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUE:
                    if (private_data.incoming_index < private_data.incoming_length)
                    {
                        *data = private_data.incoming_buffer[private_data.incoming_index];
                        private_data.incoming_index++;
                    }
                    break;
                default:
                    *data = 0xff;
                    break;
            }
            break;
        }
    }
}

static void powerup(void)
{
    cmd_byte = EXPANSION_COMMAND_NONE;
}

static void init()
{
    memset(&private_data, 0, sizeof(private_data));
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), EXPANSION_QUEUE_BUFFER_LEN);

    i2c_init(EXPANSION_PORT_I2C_INST, 100 * 1000);
    gpio_set_function(EXPANSION_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EXPANSION_PORT_I2C_SCL, GPIO_FUNC_I2C);

    lpc_interface_add_io_handler(MODXO_REGISTER_EXPANSION_COMMAND, 0xFFFE, read_handler, write_handler);
}


MODXO_TASK expansion_hdlr = {
    .init = init,
    .powerup = powerup,
    .core0_poll = expansion_poll
};
