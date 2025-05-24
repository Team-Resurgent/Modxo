/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

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

#include <stdio.h>
#include <pico.h>
#include <hardware/sync.h>

#include <modxo.h>
#include <modxo/modxo_ports.h>
#include "modxo/modxo_debug.h"
#include "modxo/lpc_interface.h"
#include "modxo/lpc_log.h"
#include <modxo/data_store.h>
#include "hardware/watchdog.h"
#include "hardware/clocks.h"

#define RUN_MODXO_HANDLERS(func) \
    {for(int _modxo_handler_idx = 0; _modxo_handler_idx < handler_count; _modxo_handler_idx++) \
        if(modxo_handlers[_modxo_handler_idx] != NULL && modxo_handlers[_modxo_handler_idx]->func != NULL) \
            modxo_handlers[_modxo_handler_idx]->func();}

extern uint8_t current_led_color;
static MODXO_TASK* modxo_handlers[15] = {NULL};
static uint8_t handler_count = 0;

bool (*modxo_debug_sp_connected)(void);

void modxo_chip_id_read(uint16_t address, uint8_t * data)
{
    *data = 0xAF;
}

void modxo_variant_id_read(uint16_t address, uint8_t * data)
{
    *data = (uint8_t)MODXO_VARIANT;
}

void modxo_poll_core1()
{
    RUN_MODXO_HANDLERS(core1_poll);
}

void modxo_poll_core0()
{
    RUN_MODXO_HANDLERS(core0_poll);
}

void modxo_lpc_reset_off()
{
    RUN_MODXO_HANDLERS(lpc_reset_off);
}

void modxo_lpc_reset_on()
{
    RUN_MODXO_HANDLERS(lpc_reset_on);
}

void software_reset()
{
    // M0+ AIRCR Register
    //*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;

    watchdog_enable(1, 1);
    while(1);
}

void modxo_shutdown()
{
    RUN_MODXO_HANDLERS(shutdown);
    // Modxo sleep
    set_sys_clock_khz(SYS_FREQ_DEFAULT, true);

    // Modxo reset
    if(!modxo_debug_sp_connected || !modxo_debug_sp_connected())
        software_reset();
}

void modxo_reset()
{
    RUN_MODXO_HANDLERS(powerup);//Modxo after LPC 3.3v  goes high (When usb is connected)
}

void modxo_init(void)
{
    RUN_MODXO_HANDLERS(init);
}

void modxo_register_handler(MODXO_TASK* handler)
{
    if(handler == NULL)
        return;

    modxo_handlers[handler_count] = handler;
    handler_count++;
}
