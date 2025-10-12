/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <stdbool.h>
#include <stdint.h>

#if PICO_RP2350
	#define SYS_FREQ_DEFAULT (150 * 1000)
	#define SYS_FREQ_IN_KHZ (266 * 1000)
#else
	#define SYS_FREQ_DEFAULT (133 * 1000)
	#define SYS_FREQ_IN_KHZ (266 * 1000)
#endif

typedef struct {
	void (*init)(void);
	void (*powerup)(void);
	void (*core0_poll)(void);
	void (*core1_poll)(void);
	void (*lpc_reset_on)(void);
	void (*lpc_reset_off)(void);
	void (*shutdown)(void);
	void (*core1_init)(void);
}MODXO_TASK;


extern MODXO_TASK* modxo_handlers[];
extern uint8_t handler_count;

#define RUN_MODXO_HANDLERS(func) \
    {for(int _modxo_handler_idx = 0; _modxo_handler_idx < handler_count; _modxo_handler_idx++) \
        if(modxo_handlers[_modxo_handler_idx] != NULL && modxo_handlers[_modxo_handler_idx]->func != NULL) \
            modxo_handlers[_modxo_handler_idx]->func();}


extern bool (*modxo_debug_sp_connected)(void);
void modxo_register_handler(MODXO_TASK* handler);
void modxo_reset(void);
void modxo_init(void);
void modxo_poll_core1(void);
void modxo_poll_core0(void);
void modxo_lpc_reset_off(void);
void modxo_lpc_reset_on(void);
void modxo_shutdown(void);
void modxo_enable_lpm(bool enable);

