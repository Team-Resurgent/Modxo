// Heap allocation handler
// Carve out a chunk of RP2040/RP2350's SRAM to be used for any purpose by the window

#include <string.h>
#include <malloc.h>
#include <lpc_mem_window.h>

#define HEAP_CMD_READ_MAX_ALLOWED_SIZE 0

// Store allocated heap addresses & sizes for each window
uint32_t heap_window_offsets[NUM_LPC_MEM_WINDOWS];
uint32_t heap_window_sizes[NUM_LPC_MEM_WINDOWS];

// Set some sensible max allowed sizes for RP2040/RP2350's SRAM
#if PICO_RP2040
uint32_t heap_max_allowed_size = 128 * 1024;
#elif PICO_RP2350
uint32_t heap_max_allowed_size = 256 * 1024;
#else
uint32_t heap_max_allowed_size = 0;
#endif

bool heap_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t heap_offset = heap_window_offsets[window_id];
	LPC_MEM_WINDOW *window = &lpc_mem_windows[window_id];

	// New window size detected, free existing buffer
	if(heap_offset && window->length != heap_window_sizes[window_id]) {
		free((void *)heap_offset);

		heap_window_offsets[window_id] = heap_window_sizes[window_id] = heap_offset = 0;
	}

	// Allocate the new buffer that matches the window size, if one doesn't exist yet
	if(!heap_offset) {
		// Bail if window is bigger than max allowed size
		if(window->length > heap_max_allowed_size) return false;

		heap_window_offsets[window_id] = heap_offset = (uint32_t)malloc(window->length);
		heap_window_sizes[window_id] = window->length;
	}

	uint32_t offset = heap_offset + (addr - window->base_addr);

	*data = *(uint8_t*)offset;

	return true;
}

bool heap_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) {
	uint32_t heap_offset = heap_window_offsets[window_id];
	LPC_MEM_WINDOW *window = &lpc_mem_windows[window_id];

	// New window size detected, free existing buffer
	if(heap_offset && window->length != heap_window_sizes[window_id]) {
		free((void *)heap_offset);

		heap_window_offsets[window_id] = heap_window_sizes[window_id] = heap_offset = 0;
	}

	// Allocate the new buffer that matches the window size, if one doesn't exist yet
	if(!heap_offset) {
		// Bail if window is bigger than max allowed size
		if(window->length > heap_max_allowed_size) return false;

		heap_window_offsets[window_id] = heap_offset = (uint32_t)malloc(window->length);
		heap_window_sizes[window_id] = window->length;
	}

	uint32_t offset = heap_offset + (addr - window->base_addr);

	(*(uint8_t*)offset) = *data;

	return true;
}

uint8_t heap_handler_control_peek(uint8_t cmd, uint8_t data) {
	switch(cmd) {

	}

	return 0;
}

uint8_t heap_handler_control_set(uint8_t cmd, uint8_t data) {
	switch(cmd) {
	case HEAP_CMD_READ_MAX_ALLOWED_SIZE:
		current_long_val = heap_max_allowed_size;
		break;
	}

	return 0;
}

uint8_t heap_handler_control(uint8_t cmd, uint8_t data, bool is_read) {
	return is_read ? heap_handler_control_peek(cmd, data) : heap_handler_control_set(cmd, data);
}

void heap_handler_powerup() {
	memset(heap_window_offsets, 0, sizeof(heap_window_offsets));
	memset(heap_window_sizes, 0, sizeof(heap_window_sizes));
}