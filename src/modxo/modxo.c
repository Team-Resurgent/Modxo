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

#include "modxo.h"
#include "modxo_ports.h"
#include "lpc/lpc_interface.h"
#include "lpc/lpc_log.h"
#include "flashrom/flashrom.h"
#include "data_store/data_store.h"
#include "superio/LPC47M152.h"
#include "superio/uart_16550.h"
#include "ws2812/ws2812.h"
#include "legacy_display/legacy_display.h"
#include "hardware/watchdog.h"

extern uint8_t current_led_color;

static void modxo_lpcmem_init()
{
    flashrom_init();
}

static void modxo_lpcio_init()
{
    lpc47m152_init();
    uart_16550_init();
    data_store_init();
    ws2812_init();
    legacy_display_init();
    modxo_ports_init();
}

void modxo_poll_core1()
{
    modxo_ports_poll();
}

void modxo_poll_core0()
{
    legacy_display_poll();
#ifdef LPC_LOGGING
    lpc_interface_poll();
#endif
    ws2812_poll();
}

void modxo_lpc_reset_off()
{
    LedColorEnum color = current_led_color;
    ws2812_set_color(LedColorOff);
    current_led_color = color;

    // Reset State Machines
    lpc_interface_start_sm();
}

void modxo_lpc_reset_on()
{
    ws2812_set_color(current_led_color);
}

void software_reset()
{
    //*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;
    watchdog_enable(1, 1);
    while(1);
}

void modxo_low_power_mode()
{
    // Modxo reset
    software_reset();

    // Modxo sleep
}

void modxo_init()
{
    lpc_interface_init();

    modxo_lpcmem_init();

    modxo_lpcio_init();
}