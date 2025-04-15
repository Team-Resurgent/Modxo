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
    {for(int i = 0; i < handler_count; i++) \
        if(modxo_handlers[i] != NULL && modxo_handlers[i]->func != NULL) \
            modxo_handlers[i]->func();}

extern uint8_t current_led_color;
static MODXO_TASK* modxo_handlers[15] = {NULL};
static uint8_t handler_count = 0;

bool (*modxo_debug_sp_connected)(void);

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case MODXO_REGISTER_CHIP_ID:
        *data = 0xAF;
        break;
    case MODXO_REGISTER_VARIANT_ID:
        *data = (uint8_t)MODXO_VARIANT;
        break;
    }
}

static void modxo_ports_init(void)
{
    lpc_interface_add_io_handler(MODXO_REGISTER_CHIP_ID, 0xFFFF, read_handler, NULL);
    lpc_interface_add_io_handler(MODXO_REGISTER_VARIANT_ID, 0xFFFF, read_handler, NULL);
}

void modxo_poll_core1()
{
    RUN_MODXO_HANDLERS(core1_poll);
}

void modxo_poll_core0()
{
#ifdef LPC_LOGGING
    lpc_interface_poll();
#endif
    RUN_MODXO_HANDLERS(core0_poll);
}

void modxo_lpc_reset_off()
{
    RUN_MODXO_HANDLERS(lpc_reset_off);
    modxo_reset();
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

void modxo_low_power_mode()
{
    // Modxo sleep
    set_sys_clock_khz(SYS_FREQ_DEFAULT, true);

    // Modxo reset
    if(!modxo_debug_sp_connected || !modxo_debug_sp_connected())
        software_reset();
}

void modxo_reset()
{
    RUN_MODXO_HANDLERS(reset);
}

void modxo_init(void)
{
    modxo_ports_init();
    modxo_register_handler(&lpc_interface_hdlr);
    modxo_register_handler(&data_store_handler);
    RUN_MODXO_HANDLERS(init);
}

void modxo_register_handler(void* handler)
{
    MODXO_TASK* modxo_handler = (MODXO_TASK*)handler;

    if(modxo_handler == NULL)
        return;
    
    modxo_handlers[handler_count] = modxo_handler;
    handler_count++;
}
