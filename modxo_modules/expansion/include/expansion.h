#pragma once
#include <modxo.h>
#include <stdint.h>

// Expansion Packet Protocol

#define EXPANSION_COMMAND_NONE 0

// IO Write

#define EXPANSION_COMMAND_REQUEST_I2C_ADDRESS_EXIST 1 // 2 byte command,data (command + address)
#define EXPANSION_COMMAND_REQUEST_INCOMING_VALUES 2 // 2 byte command (command + address)
#define EXPANSION_COMMAND_SET_INCOMING_INDEX 32 // 2 byte command,data (command + index)
#define EXPANSION_COMMAND_SET_OUTGOING_LENGTH 33 // 2 byte command,data (command + length)
#define EXPANSION_COMMAND_SET_OUTGOING_INDEX 34 // 2 byte command,data (command + index)
#define EXPANSION_COMMAND_SET_OUTGOING_VALUE 35 // 2 byte command,data (command + sets value at index + increments)
#define EXPANSION_COMMAND_SEND_OUTGOING_VALUES 36 // 2 byte command (command + address)

// IO Read

#define EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXIST_READY 64 // 1 byte command (returns 1 = ready, 0 = not ready)
#define EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUES_READY 65 // 1 byte command (returns 1 = ready, 0 = not ready)
#define EXPANSION_COMMAND_GET_RESPONSE_I2C_ADDRESS_EXISTS 66 // 1 byte command (returns 1 = exists, 0 = doesnt exist)
#define EXPANSION_COMMAND_GET_RESPONSE_INCOMING_LENGTH 67 // 1 byte command (returns length)
#define EXPANSION_COMMAND_GET_RESPONSE_INCOMING_VALUE 68 // 1 byte command (returns value at index + increments)

extern MODXO_TASK expansion_hdlr;
