
#include <string.h>
#include <stdio.h>

#include <modxo.h>
#include <modxo/lpc_interface.h>
#include <lpc_mapper.h>

LPC_MAPPER lpc_mappers[NUM_LPC_MAPPERS];

bool lpc_mapper_enabled = false;
uint8_t current_lpc_mapper_cmd;
uint32_t current_long_val;
uint8_t current_mapper_id;
uint8_t current_mapper_hdlr_cmd;
uint8_t current_mapper_hdlr_data;

bool shortcut_enabled = false;
uint8_t shortcut_mapper_id = 0;
uint32_t shortcut_base_addr = 0;
uint32_t shortcut_buffer_size = 0;
uint8_t *shortcut_buffer = NULL;


// Returns true if there is a custom enabled handler for the given address
// Returns false if there is no handler for the given address range and
//    fallback to default flashrom handler
bool lpc_mapper_custom_rw_handler(uint32_t addr, uint8_t *data, bool is_read) {
	LPC_MAPPER *mapper;

	// Handlers can setup an arbitrary read buffer that bypasses the main loop.
	// If none of the conditions are meet or the incoming addr is out-of-bounds,
	// then fall-thru to main loop
	if(1
		&& shortcut_enabled // Having this first minimizes opcode count if not enabled
		&& shortcut_mapper_id == current_mapper_id // Only one mapper can own the shortcut
		&& is_read
		&& shortcut_buffer_size
		&& shortcut_buffer
	) {
		uint32_t offset = addr - shortcut_base_addr;

		if(offset < shortcut_buffer_size) {
			*data = *(uint8_t*)(shortcut_buffer + offset);
			return true;
		}
	}

	for(size_t i = 0; i < NUM_LPC_MAPPERS; i++) {
		uint32_t start;
		uint32_t end;
		uint32_t len;

		mapper = &lpc_mappers[i];

		start = mapper->base_addr;
		len = mapper->length;
		end = start + len - 1;

		if(addr >= start && addr <= end) {
			if(is_read && mapper->read && (mapper->flags & LPC_MAPPER_FLAG_READ_ENABLED)) {
				return mapper->read(addr, data, i);
			} else if(mapper->write && (mapper->flags & LPC_MAPPER_FLAG_WRITE_ENABLED)) {
				return mapper->write(addr, data, i);
			}
		}
	}

	return false;
}

bool lpc_mapper_custom_read_handler(uint32_t addr, uint8_t *data) {
	return lpc_mapper_custom_rw_handler(addr, data, true);
}

bool lpc_mapper_custom_write_handler(uint32_t addr, uint8_t *data) {
	return lpc_mapper_custom_rw_handler(addr, data, false);
}

void setup_handler(uint8_t type) {
	LPC_MAPPER *mapper = &lpc_mappers[current_mapper_id];

	switch (type) {
#ifdef PSRAM_ENABLE
	case LPC_MAPPER_TYPE_PSRAM:
		mapper->read = psram_memread_handler;
		mapper->write = psram_memwrite_handler;
		mapper->control = psram_handler_control;
		break;
#endif
	case LPC_MAPPER_TYPE_INTERNAL_RAM:
		mapper->read = int_ram_memread_handler;
		mapper->write = int_ram_memwrite_handler;
		mapper->control = int_ram_handler_control;
		break;

	case LPC_MAPPER_TYPE_RNG:
		mapper->read = rng_memread_handler;
		mapper->write = rng_memwrite_handler;
		mapper->control = rng_handler_control;
		break;

	case LPC_MAPPER_TYPE_ECHO:
		mapper->read = echo_memread_handler;
		mapper->write = echo_memwrite_handler;
		mapper->control = echo_handler_control;
		break;

	case LPC_MAPPER_TYPE_HEAP:
		mapper->read = heap_memread_handler;
		mapper->write = heap_memwrite_handler;
		mapper->control = heap_handler_control;
		break;

	case LPC_MAPPER_TYPE_SDCARD:
		mapper->read = sdcard_memread_handler;
		mapper->write = sdcard_memwrite_handler;
		mapper->control = sdcard_handler_control;
		break;

	case LPC_MAPPER_TYPE_EXPANSION:
		mapper->read = expansion_memread_handler;
		mapper->write = expansion_memwrite_handler;
		mapper->control = expansion_handler_control;
		break;

	case LPC_MAPPER_TYPE_FLASH:
		mapper->read = flash_memread_handler;
		mapper->write = flash_memwrite_handler;
		mapper->control = flash_handler_control;
		break;

	default:
		mapper->read = NULL;
		mapper->write = NULL;
		mapper->control = NULL;
		break;
	}
}

uint8_t peek_mapper_type() {
	LPC_MAPPER *mapper = &lpc_mappers[current_mapper_id];

#ifdef PSRAM_ENABLE
	if(mapper->read == psram_memread_handler) {
		return LPC_MAPPER_TYPE_PSRAM;
	}
	else 
#endif
	if(mapper->read == int_ram_memread_handler) {
		return LPC_MAPPER_TYPE_INTERNAL_RAM;
	}
	else if(mapper->read == rng_memread_handler) {
		return LPC_MAPPER_TYPE_RNG;
	}
	else if(mapper->read == echo_memread_handler) {
		return LPC_MAPPER_TYPE_ECHO;
	}
	else if(mapper->read == heap_memread_handler) {
		return LPC_MAPPER_TYPE_HEAP;
	}
	else if(mapper->read == sdcard_memread_handler) {
		return LPC_MAPPER_TYPE_SDCARD;
	}
	else if(mapper->read == expansion_memread_handler) {
		return LPC_MAPPER_TYPE_EXPANSION;
	}

	return LPC_MAPPER_TYPE_NONE;
}

void send_command(uint8_t data) {
	LPC_MAPPER *mapper = &lpc_mappers[current_mapper_id];

	switch (current_lpc_mapper_cmd) {
	case LPC_MAPPER_CMD_ENABLE:
		lpc_mapper_enabled = data;
		break;

	case LPC_MAPPER_CMD_CUR_MAPPER_ID:
		if(data > NUM_LPC_MAPPERS - 1) data = NUM_LPC_MAPPERS - 1;
		if(data < 0) data = 0;
		current_mapper_id = data;
		break;

	case LPC_MAPPER_CMD_MAPPER_BASE_ADDR:
		mapper->base_addr = current_long_val;
		break;

	case LPC_MAPPER_CMD_MAPPER_LENGTH:
		mapper->length = (current_long_val == 0) ? 1 : current_long_val;
		break;

	case LPC_MAPPER_CMD_MAPPER_TYPE:
		setup_handler(data);
		break;

	case LPC_MAPPER_CMD_MAPPER_FLAGS:
		mapper->flags = data;
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL:
		if(mapper->control) mapper->control(current_mapper_hdlr_cmd, data, false);
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL_CMD:
		current_mapper_hdlr_cmd = data;
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL_DATA:
		current_mapper_hdlr_data = data;
		break;

	case LPC_MAPPER_CMD_SHORTCUT_ENABLE:
		shortcut_enabled = data;
		break;
	}
}

uint8_t peek_command() {
	LPC_MAPPER *mapper = &lpc_mappers[current_mapper_id];

	switch (current_lpc_mapper_cmd) {
	case LPC_MAPPER_CMD_ENABLE:
		return lpc_mapper_enabled;
		break;

	case LPC_MAPPER_CMD_CUR_MAPPER_ID:
		return current_mapper_id;
		break;

	case LPC_MAPPER_CMD_MAPPER_BASE_ADDR:
		current_long_val = mapper->base_addr;
		break;

	case LPC_MAPPER_CMD_MAPPER_LENGTH:
		current_long_val = mapper->length;
		break;

	case LPC_MAPPER_CMD_MAPPER_TYPE:
		return peek_mapper_type();
		break;

	case LPC_MAPPER_CMD_MAPPER_FLAGS:
		return mapper->flags;
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL:
		if(mapper->control) return mapper->control(current_mapper_hdlr_cmd, current_mapper_hdlr_data, true);
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL_CMD:
		return current_mapper_hdlr_cmd;
		break;

	case LPC_MAPPER_CMD_MAPPER_CTRL_DATA:
		return current_mapper_hdlr_data;
		break;

	case LPC_MAPPER_CMD_SHORTCUT_ENABLE:
		return shortcut_enabled;
		break;
	}

	return 0xff;
}

void lpc_mapper_read_handler(uint16_t addr, uint8_t *data) {
	switch (addr) {
	case LPC_MAPPER_VERSION_PORT:
		*data = LPC_MAPPER_VERSION;
		break;

	case LPC_MAPPER_CMD_PORT:
		*data = peek_command();
		break;

	case LPC_MAPPER_DATA_PORT:
		break;

	default: // assume this is a long (32-bit) addr read cmd
		uint8_t shift = (addr - LPC_MAPPER_LONG_DATA_PORT_BASE) * 8;
		if(shift > 24 || shift < 0) break;

		*data = (uint8_t)(current_long_val >> shift);
	}
}

static void lpc_mapper_write_handler(uint16_t addr, uint8_t *data) {
	switch (addr) {
	case LPC_MAPPER_CMD_PORT:
		current_lpc_mapper_cmd = *data;
		break;

	case LPC_MAPPER_DATA_PORT:
		send_command(*data);
		break;

	default: // assume this is a long (32-bit) addr write cmd
		uint8_t shift = (addr - LPC_MAPPER_LONG_DATA_PORT_BASE) * 8;
		if(shift > 24 || shift < 0) break;

		current_long_val &= ~(0xFF << shift); // reset current bits to 0
		current_long_val |= (*data << shift);
	}
}

static void run_handler_powerups() {
	rng_handler_powerup();
	int_ram_handler_powerup();
	echo_handler_powerup();
	heap_handler_powerup();
	sdcard_handler_powerup();
	expansion_handler_powerup();
	flash_handler_powerup();
}

static void powerup() {
	memset(lpc_mappers, 0, sizeof(lpc_mappers));
	current_mapper_id = 0;
	current_long_val = 0;
	current_lpc_mapper_cmd = 0;
	current_mapper_hdlr_cmd = 0;
	current_mapper_hdlr_data = 0;
	lpc_mapper_enabled = false;

	shortcut_enabled = false;
	shortcut_mapper_id = 0;
	shortcut_base_addr = 0;
	shortcut_buffer_size = 0;
	shortcut_buffer = NULL;

	run_handler_powerups();
}

static void init() {
	lpc_interface_add_io_handler(LPC_MAPPER_IO_BASE, 0xFFF8, lpc_mapper_read_handler, lpc_mapper_write_handler);
}

static void lpc_mapper_poll() {
	sdcard_handler_poll();
	expansion_handler_poll();
}

MODXO_TASK lpc_mapper_hdlr = {
	.init = init,
	.powerup = powerup,
	.core1_poll = lpc_mapper_poll,
};