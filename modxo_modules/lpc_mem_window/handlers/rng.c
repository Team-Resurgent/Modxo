// Random Number Generator - read a random value from the window,
// reading from (addr % 8 == 0) will generate a new value and will
// take longer to return. Writes will overwrite the byte at the
// given (addr % 8)

#include "pico.h"
#include "pico/rand.h"

#include <string.h>
#include <modxo/lpc_mem_window.h>

#define RNG_CMD_WIN_RESET 0

uint64_t rand_val;

bool rng_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint8_t mod = addr % sizeof(rand_val);
	uint8_t shift = mod * 8;

	if(mod == 0) rand_val = get_rand_64();
	*data = (uint8_t)(rand_val >> shift);

	return true;
}

bool rng_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint8_t mod = addr % sizeof(rand_val);
	uint8_t shift = mod * 8;

	rand_val &= ~(0xFF << shift);
	rand_val |= (*data << shift);

	return true;
}

uint8_t rng_handler_control_peek(uint8_t cmd, uint8_t data) {
	switch(cmd) {

	}

	return 0;
}

uint8_t rng_handler_control_set(uint8_t cmd, uint8_t data) {
	switch(cmd) {
	case RNG_CMD_WIN_RESET:
		rand_val = get_rand_64();
		break;
	}

	return 0;
}

uint8_t rng_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? rng_handler_control_peek(cmd, data) : rng_handler_control_set(cmd, data);
}

void rng_handler_powerup() {
	rand_val = get_rand_64();
}