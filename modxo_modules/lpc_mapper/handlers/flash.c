// Raw external flash access handler
// Send raw flash commands to the device externally connected to the QSPI bus
//      via flash_do_cmd()

#include "hardware/flash.h"
#include <string.h>
#include <lpc_mapper.h>

#define FLASH_BUFFERS_SIZE 256

#define FLASH_CMD_MAX_BUFFER_SIZE 0
#define FLASH_CMD_SEND_TXBUF 1

uint8_t flash_txbuf[NUM_LPC_MAPPERS][FLASH_BUFFERS_SIZE];
uint8_t flash_rxbuf[NUM_LPC_MAPPERS][FLASH_BUFFERS_SIZE];


bool flash_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id) {
	LPC_MAPPER *mapper = &lpc_mappers[mapper_id];
	uint32_t offset = addr - mapper->base_addr;
	if(offset >= FLASH_BUFFERS_SIZE) return false;

	*data = flash_rxbuf[mapper_id][offset];

	return true;
}

bool flash_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id) {
	LPC_MAPPER *mapper = &lpc_mappers[mapper_id];
	uint32_t offset = addr - mapper->base_addr;
	if(offset >= FLASH_BUFFERS_SIZE) return false;

	flash_txbuf[mapper_id][offset] = *data;

	return true;
}

uint8_t flash_handler_control_peek(uint8_t cmd, uint8_t data) {
	switch(cmd) {

	}

	return 0;
}

uint8_t flash_handler_control_set(uint8_t cmd, uint8_t data) {
	LPC_MAPPER *mapper = &lpc_mappers[current_mapper_id];

	switch(cmd) {
	case FLASH_CMD_MAX_BUFFER_SIZE:
		current_long_val = FLASH_BUFFERS_SIZE;
		break;

	case FLASH_CMD_SEND_TXBUF:
		size_t sz = min(mapper->length, FLASH_BUFFERS_SIZE);
		if(data) sz = min(data, sz);
		flash_do_cmd(flash_txbuf[current_mapper_id], flash_rxbuf[current_mapper_id], sz);
		break;
	}

	return 0;
}

uint8_t flash_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? flash_handler_control_peek(cmd, data) : flash_handler_control_set(cmd, data);
}

void flash_handler_powerup() {
	memset(flash_txbuf, 0, sizeof(flash_txbuf));
	memset(flash_rxbuf, 0, sizeof(flash_rxbuf));
}