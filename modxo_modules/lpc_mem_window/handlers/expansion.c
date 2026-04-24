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
#include <modxo.h>
#include <modxo/modxo_queue.h>
#include <modxo/modxo_ports.h>
#include <modxo/lpc_interface.h>
#include <lpc_mem_window.h>

#include <pico.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>
#include <hardware/sync.h>

#include <modxo_pinout.h>

#define EXPANSION_COMMAND_NONE 0
#define EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST 1 
#define EXPANSION_COMMAND_RESPONSE_I2C_ADDRESS_EXIST_READY 2
#define EXPANSION_COMMAND_RESPONSE_I2C_ADDRESS_EXIST_RESULT 3
#define EXPANSION_COMMAND_REQUEST_RECEIVE_INCOMING_VALUES 4
#define EXPANSION_COMMAND_RESPONSE_RECEIVE_INCOMING_VALUES_READY 5 
#define EXPANSION_COMMAND_RESPONSE_RECEIVE_INCOMING_VALUES_RESULT 6 // sets current long to length 
#define EXPANSION_COMMAND_REQUEST_SEND_OUTGOING_VALUES 7 // Set current long to length
#define EXPANSION_COMMAND_RESPONSE_SEND_OUTGOING_VALUES_READY 8 
#define EXPANSION_COMMAND_RESPONSE_SEND_OUTGOING_VALUES_RESULT 9 
#define EXPANSION_COMMAND_SET_PAYLOAD_TYPE 10

#define EXPANSION_RESULT_IDLE 0 
#define EXPANSION_RESULT_OK 1
#define EXPANSION_RESULT_TIMEOUT 2
#define EXPANSION_RESULT_INVALID_LEN 3

#define EXPANSION_PAYLOAD_TYPE_NONE 0
#define EXPANSION_PAYLOAD_TYPE_INCOMING 1
#define EXPANSION_PAYLOAD_TYPE_OUTGOING 2

#define EXPANSION_QUEUE_BUFFER_LEN 8
#define EXPANSION_MAX_BUFFER_LEN 1024
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

static struct
{
    MODXO_QUEUE_ITEM_T buffer[EXPANSION_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;

    uint8_t i2c_address_exists_address;
    uint8_t i2c_address_exists_ready;
    uint8_t i2c_address_exists_result;

    uint8_t incoming_values_address;
    uint8_t incoming_values_ready;
    uint8_t incoming_values_result;
    uint16_t incoming_values_length;
    uint8_t incoming_values_buffer[EXPANSION_MAX_BUFFER_LEN];

    uint8_t outgoing_values_address;
    uint8_t outgoing_values_ready;
    uint8_t outgoing_values_result;
    uint16_t outgoing_values_length;
    uint8_t outgoing_values_buffer[EXPANSION_MAX_BUFFER_LEN];

    uint8_t payload_type;

} private_data;

void expansion_queue_command(uint8_t cmd, uint8_t data)
{
    if (cmd == EXPANSION_COMMAND_NONE) {
        return;
    }
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.data.cmd = (uint8_t)cmd;
    _item.data.param1 = (uint8_t)data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

void expansion_does_address_exist()
{
    uint8_t temp = 0;
    private_data.i2c_address_exists_result = (i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, private_data.i2c_address_exists_address, (uint8_t*)&temp, 1, false, EXPANSION_TIMEOUT_US) >= 0) ? 1 : 0;
    private_data.i2c_address_exists_ready = 1;
}

void expansion_receive_incoming_buffer()
{
    uint8_t length_size = sizeof(private_data.incoming_values_length);

    if (i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, private_data.incoming_values_address, (uint8_t*)&private_data.incoming_values_length, length_size, false, EXPANSION_TIMEOUT_US) != length_size) { 
        private_data.incoming_values_result = EXPANSION_RESULT_TIMEOUT; 
        private_data.incoming_values_ready = 1; 
        return; 
    }

    if (private_data.incoming_values_length > EXPANSION_MAX_BUFFER_LEN) {
        private_data.incoming_values_result = EXPANSION_RESULT_INVALID_LEN;
        private_data.outgoing_values_ready = 1;
        return;
    }

    if (private_data.incoming_values_length > 0 && i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, private_data.incoming_values_address, private_data.incoming_values_buffer, private_data.incoming_values_length, false, EXPANSION_TIMEOUT_US) != private_data.incoming_values_length) { 
        private_data.incoming_values_result = EXPANSION_RESULT_TIMEOUT; 
        private_data.incoming_values_ready = 1; 
        return; 
    }

    private_data.incoming_values_result = EXPANSION_RESULT_OK; 
    private_data.incoming_values_ready = 1;
}

void expansion_send_outgoing_buffer()
{
    if (private_data.outgoing_values_length > EXPANSION_MAX_BUFFER_LEN) {
        private_data.outgoing_values_result = EXPANSION_RESULT_INVALID_LEN;
        private_data.outgoing_values_ready = 1;
        return;
    }

    uint8_t length_size = sizeof(private_data.outgoing_values_length);

    if (i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, private_data.outgoing_values_address, (uint8_t*)&private_data.outgoing_values_length, length_size, false, EXPANSION_TIMEOUT_US) != length_size) {
        private_data.outgoing_values_result = EXPANSION_RESULT_TIMEOUT;
        private_data.outgoing_values_ready = 1;
    }

    if (private_data.outgoing_values_length > 0 && i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, private_data.outgoing_values_address, private_data.outgoing_values_buffer, private_data.outgoing_values_length, false, EXPANSION_TIMEOUT_US) != private_data.outgoing_values_length) {
        private_data.outgoing_values_result = EXPANSION_RESULT_TIMEOUT;
        private_data.outgoing_values_ready = 1;
        return;
    }
        
    private_data.outgoing_values_result = EXPANSION_RESULT_OK;
    private_data.outgoing_values_ready = 1;
}

bool expansion_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) 
{
    uint32_t offset = (addr - lpc_mem_windows[window_id].base_addr);
    offset = offset & (lpc_mem_windows[window_id].length - 1);

    if (offset < EXPANSION_MAX_BUFFER_LEN)
    {
        if (private_data.payload_type == EXPANSION_PAYLOAD_TYPE_INCOMING)
        {
            *data = private_data.incoming_values_buffer[offset];
            return true;
        }
        else if (private_data.payload_type == EXPANSION_PAYLOAD_TYPE_OUTGOING)
        {
            *data = private_data.outgoing_values_buffer[offset];
            return true;
        }
    }

    *data = 0;
	return true;
}

bool expansion_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) 
{
    uint32_t offset = (addr - lpc_mem_windows[window_id].base_addr);
    offset = offset & (lpc_mem_windows[window_id].length - 1);

    if (offset < EXPANSION_MAX_BUFFER_LEN)
    {
        if (private_data.payload_type == EXPANSION_PAYLOAD_TYPE_OUTGOING)
        {
            private_data.outgoing_values_buffer[offset]  = *data;
            return true;
        }
    }

	return true;
}

uint8_t expansion_handler_control_peek(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case EXPANSION_COMMAND_RESPONSE_I2C_ADDRESS_EXIST_READY:
            return private_data.i2c_address_exists_ready;
        case EXPANSION_COMMAND_RESPONSE_I2C_ADDRESS_EXIST_RESULT:
            return private_data.i2c_address_exists_result;
        case EXPANSION_COMMAND_RESPONSE_RECEIVE_INCOMING_VALUES_READY:
            return private_data.incoming_values_ready;
        case EXPANSION_COMMAND_RESPONSE_RECEIVE_INCOMING_VALUES_RESULT:
            current_long_val = private_data.incoming_values_length;
            return private_data.incoming_values_result;
        case EXPANSION_COMMAND_RESPONSE_SEND_OUTGOING_VALUES_READY:
            return private_data.outgoing_values_ready;
        case EXPANSION_COMMAND_RESPONSE_SEND_OUTGOING_VALUES_RESULT:
            return private_data.outgoing_values_result;
	}
	return 0;
}

uint8_t expansion_handler_control_set(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST:
            private_data.i2c_address_exists_address = data & 0x7f;
            private_data.i2c_address_exists_ready = 0;
            private_data.i2c_address_exists_result = EXPANSION_RESULT_IDLE;
            expansion_queue_command(cmd, data);
            break;
        case EXPANSION_COMMAND_REQUEST_RECEIVE_INCOMING_VALUES:
            private_data.incoming_values_address = data & 0x7f;
            private_data.incoming_values_ready = 0;
            private_data.incoming_values_result = EXPANSION_RESULT_IDLE;
            private_data.incoming_values_length = 0;
            expansion_queue_command(cmd, data);
            break;
        case EXPANSION_COMMAND_REQUEST_SEND_OUTGOING_VALUES:
            private_data.outgoing_values_address = data & 0x7f;
            private_data.outgoing_values_ready = 0;
            private_data.outgoing_values_result = EXPANSION_RESULT_IDLE;
            private_data.outgoing_values_length = current_long_val;
            expansion_queue_command(cmd, data);
            break;
        case EXPANSION_COMMAND_SET_PAYLOAD_TYPE:
            private_data.payload_type = data;
            break;
	}
	return 0;
}

uint8_t expansion_handler_control(uint8_t cmd, uint8_t data, bool is_read) 
{
	return is_read ? expansion_handler_control_peek(cmd, data) : expansion_handler_control_set(cmd, data);
}

void expansion_handler_poll()
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
                    expansion_does_address_exist();
                    break;
                case EXPANSION_COMMAND_REQUEST_RECEIVE_INCOMING_VALUES:
                    expansion_receive_incoming_buffer();
                    break; 
                case EXPANSION_COMMAND_REQUEST_SEND_OUTGOING_VALUES:
                    expansion_send_outgoing_buffer();
                    break;
            }
        }
    }
}

void expansion_handler_powerup() 
{
    memset(&private_data, 0, sizeof(private_data));
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), EXPANSION_QUEUE_BUFFER_LEN);

    i2c_init(EXPANSION_PORT_I2C_INST, 400 * 1000);
    gpio_set_function(EXPANSION_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EXPANSION_PORT_I2C_SCL, GPIO_FUNC_I2C);
}