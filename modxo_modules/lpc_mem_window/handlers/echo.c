// Simple single byte echo handler, great for testing

#include <string.h>
#include <modxo/lpc_mem_window.h>

uint8_t echo_val;

bool echo_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	*data = echo_val;
	return true;
}

bool echo_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	echo_val = *data;
	return true;
}


uint8_t echo_handler_control_peek(uint8_t cmd, uint8_t data) {
	switch(cmd) {

	}

	return 0;
}

uint8_t echo_handler_control_set(uint8_t cmd, uint8_t data) {
	switch(cmd) {

	}

	return 0;
}

uint8_t echo_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? echo_handler_control_peek(cmd, data) : echo_handler_control_set(cmd, data);
}


void echo_handler_powerup() {
	echo_val = 0;
}