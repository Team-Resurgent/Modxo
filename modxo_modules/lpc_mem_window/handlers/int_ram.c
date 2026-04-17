// Internal (rp2040/rp2350) ram handler, use this handler with caution

#include <string.h>
#include <modxo/lpc_mem_window.h>

#define INT_RAM_CMD_WIN_OFFSET 0

uint32_t int_ram_window_offsets[NUM_LPC_MEM_WINDOWS];

bool int_ram_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t int_ram_offset = int_ram_window_offsets[window_id];
	volatile uint8_t *int_mem = (uint8_t*)0;
	uint32_t offset = int_ram_offset + (addr - lpc_mem_windows[window_id].base_addr);

	*data = int_mem[offset];

	return true;
}

bool int_ram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t int_ram_offset = int_ram_window_offsets[window_id];
	volatile uint8_t *int_mem = (uint8_t*)0;
	uint32_t offset = int_ram_offset + (addr - lpc_mem_windows[window_id].base_addr);

	int_mem[offset] = *data;

	return true;
}


uint8_t int_ram_handler_control_peek(uint8_t cmd, uint8_t data) {
	switch(cmd) {
	case INT_RAM_CMD_WIN_OFFSET:
		current_long_val = int_ram_window_offsets[current_window_id];
		break;
	}

	return 0;
}

uint8_t int_ram_handler_control_set(uint8_t cmd, uint8_t data) {
	switch(cmd) {
	case INT_RAM_CMD_WIN_OFFSET:
		int_ram_window_offsets[current_window_id] = current_long_val;
		break;
	}

	return 0;
}

uint8_t int_ram_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? int_ram_handler_control_peek(cmd, data) : int_ram_handler_control_set(cmd, data);
}

void int_ram_handler_powerup() {
	memset(int_ram_window_offsets, 0, sizeof(int_ram_window_offsets));
}

