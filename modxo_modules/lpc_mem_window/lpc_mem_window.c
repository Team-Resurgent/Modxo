
#include <string.h>
#include <stdio.h>

#include <modxo.h>
#include <modxo/lpc_interface.h>
#include <lpc_mem_window.h>

LPC_MEM_WINDOW lpc_mem_windows[NUM_LPC_MEM_WINDOWS];

uint8_t lpc_mem_window_enabled = false;
uint8_t current_lpc_mem_win_cmd;
uint32_t current_long_val;
uint8_t current_window_id;
uint8_t current_win_hdlr_cmd;



// Returns true if there is a custom enabled handler for the given address
// Returns false if there is no handler for the given address range and
//    fallback to default flashrom handler
bool lpc_mem_window_custom_rw_handler(uint32_t addr, uint8_t *data, bool is_read) {
	LPC_MEM_WINDOW *window;

	for(size_t i = 0; i < NUM_LPC_MEM_WINDOWS; i++) {
		uint32_t start;
		uint32_t end;
		uint32_t len;

		window = &lpc_mem_windows[i];

		start = window->base_addr;
		len = window->length;
		end = start + len - 1;

		if(addr >= start && addr <= end) {
			if(is_read && window->read && (window->flags & LPC_MEM_WIN_FLAG_READ_ENABLED)) {
				return window->read(addr, data, i);
			} else if(window->write && (window->flags & LPC_MEM_WIN_FLAG_WRITE_ENABLED)) {
				return window->write(addr, data, i);
			}
		}
	}

	return false;
}

bool lpc_mem_window_custom_read_handler(uint32_t addr, uint8_t *data) {
	return lpc_mem_window_custom_rw_handler(addr, data, true);
}

bool lpc_mem_window_custom_write_handler(uint32_t addr, uint8_t *data) {
	return lpc_mem_window_custom_rw_handler(addr, data, false);
}

void setup_handler(uint8_t type) {
	LPC_MEM_WINDOW *window = &lpc_mem_windows[current_window_id];

	switch (type) {
#ifdef PSRAM_ENABLE
	case LPC_MEM_WIN_TYPE_PSRAM:
		window->read = psram_memread_handler;
		window->write = psram_memwrite_handler;
		window->control = psram_handler_control;
		break;
#endif
	case LPC_MEM_WIN_TYPE_INTERNAL_RAM:
		window->read = int_ram_memread_handler;
		window->write = int_ram_memwrite_handler;
		window->control = int_ram_handler_control;
		break;

	case LPC_MEM_WIN_TYPE_RNG:
		window->read = rng_memread_handler;
		window->write = rng_memwrite_handler;
		window->control = rng_handler_control;
		break;

	case LPC_MEM_WIN_TYPE_ECHO:
		window->read = echo_memread_handler;
		window->write = echo_memwrite_handler;
		window->control = echo_handler_control;
		break;

	case LPC_MEM_WIN_TYPE_SDCARD:
		window->read = sdcard_memread_handler;
		window->write = sdcard_memwrite_handler;
		window->control = sdcard_handler_control;
		break;

	default:
		window->read = NULL;
		window->write = NULL;
		window->control = NULL;
		break;
	}
}

uint8_t peek_window_type() {
	LPC_MEM_WINDOW *window = &lpc_mem_windows[current_window_id];

#ifdef PSRAM_ENABLE
	if(window->read == psram_memread_handler) {
		return LPC_MEM_WIN_TYPE_PSRAM;
	}
	else 
#endif
	if(window->read == int_ram_memread_handler) {
		return LPC_MEM_WIN_TYPE_INTERNAL_RAM;
	}
	else if(window->read == rng_memread_handler) {
		return LPC_MEM_WIN_TYPE_RNG;
	}
	else if(window->read == echo_memread_handler) {
		return LPC_MEM_WIN_TYPE_ECHO;
	}
	else if(window->read == sdcard_memread_handler) {
		return LPC_MEM_WIN_TYPE_SDCARD;
	}

	return 0;
}

void send_command(uint8_t data) {
	LPC_MEM_WINDOW *window = &lpc_mem_windows[current_window_id];

	switch (current_lpc_mem_win_cmd) {
	case LPC_MEM_WIN_CMD_ENABLE:
		lpc_mem_window_enabled = data;
		break;

	case LPC_MEM_WIN_CMD_CUR_WIN_ID:
		if(data > NUM_LPC_MEM_WINDOWS - 1) data = NUM_LPC_MEM_WINDOWS - 1;
		if(data < 0) data = 0;
		current_window_id = data;
		break;

	case LPC_MEM_WIN_CMD_WIN_BASE_ADDR:
		window->base_addr = current_long_val;
		break;

	case LPC_MEM_WIN_CMD_WIN_LENGTH:
		window->length = (current_long_val == 0) ? 1 : current_long_val;
		break;

	case LPC_MEM_WIN_CMD_WIN_TYPE:
		setup_handler(data);
		break;

	case LPC_MEM_WIN_CMD_WIN_FLAGS:
		window->flags = data;
		break;

	case LPC_MEM_WIN_CMD_WIN_CTRL:
		if(window->control) window->control(current_win_hdlr_cmd, data, false);
		break;

	case LPC_MEM_WIN_CMD_WIN_CTRL_CMD:
		current_win_hdlr_cmd = data;
		break;
	}
}

uint8_t peek_command() {
	LPC_MEM_WINDOW *window = &lpc_mem_windows[current_window_id];

	switch (current_lpc_mem_win_cmd) {
	case LPC_MEM_WIN_CMD_ENABLE:
		return lpc_mem_window_enabled;
		break;

	case LPC_MEM_WIN_CMD_CUR_WIN_ID:
		return current_window_id;
		break;

	case LPC_MEM_WIN_CMD_WIN_BASE_ADDR:
		current_long_val = window->base_addr;
		break;

	case LPC_MEM_WIN_CMD_WIN_LENGTH:
		current_long_val = window->length;
		break;

	case LPC_MEM_WIN_CMD_WIN_TYPE:
		return peek_window_type();
		break;

	case LPC_MEM_WIN_CMD_WIN_FLAGS:
		return window->flags;
		break;

	case LPC_MEM_WIN_CMD_WIN_CTRL:
		if(window->control) return window->control(current_win_hdlr_cmd, 0, true);
		break;

	case LPC_MEM_WIN_CMD_WIN_CTRL_CMD:
		return current_win_hdlr_cmd;
		break;
	}

	return 0xff;
}

void lpc_mem_win_read_handler(uint16_t addr, uint8_t *data) {
	switch (addr) {
	case LPC_MEM_WIN_VERSION_PORT:
		*data = LPC_MEM_WIN_VERSION;
		break;

	case LPC_MEM_WIN_CMD_PORT:
		*data = peek_command();
		break;

	case LPC_MEM_WIN_DATA_PORT:
		break;

	default: // assume this is a long (32-bit) addr read cmd
		uint8_t shift = (addr - LPC_MEM_WIN_LONG_DATA_PORT_BASE) * 8;
		if(shift > 24 || shift < 0) break;

		*data = (uint8_t)(current_long_val >> shift);
	}
}

void lpc_mem_win_write_handler(uint16_t addr, uint8_t *data) {
	switch (addr) {
	case LPC_MEM_WIN_CMD_PORT:
		current_lpc_mem_win_cmd = *data;
		break;

	case LPC_MEM_WIN_DATA_PORT:
		send_command(*data);
		break;

	default: // assume this is a long (32-bit) addr write cmd
		uint8_t shift = (addr - LPC_MEM_WIN_LONG_DATA_PORT_BASE) * 8;
		if(shift > 24 || shift < 0) break;

		current_long_val &= ~(0xFF << shift); // reset current bits to 0
		current_long_val |= (*data << shift);
	}
}

void run_handler_powerups() {
	rng_handler_powerup();
	int_ram_handler_powerup();
	echo_handler_powerup();
	sdcard_handler_powerup();
}

void powerup() {
	memset(lpc_mem_windows, 0, sizeof(lpc_mem_windows));
	current_window_id = 0;
	current_long_val = 0;
	current_lpc_mem_win_cmd = 0;
	current_win_hdlr_cmd = 0;
	lpc_mem_window_enabled = false;

	run_handler_powerups();
}

void init() {
	lpc_interface_add_io_handler(LPC_MEM_WIN_IO_BASE, 0xFFF8, lpc_mem_win_read_handler, lpc_mem_win_write_handler);		

	sdcard_handler_init();
}

void core0_poll() {
	sdcard_handler_poll();
}

MODXO_TASK lpc_mem_window_hdlr = {
	.init = init,
	.powerup = powerup,
    .core0_poll = core0_poll
};

