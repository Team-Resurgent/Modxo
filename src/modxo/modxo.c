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
#include <pico/platform.h>
#include <hardware/sync.h>

#include "modxo.h"
#include "modxo_ports.h"
#include "lpc/lpc_interface.h"
#include "lpc/lpc_log.h"
#include "flashrom/flashrom.h"
#include "superio/DATA_STORE.h"
#include "superio/LPC47M152.h"
#include "superio/UART_16550.h"
#include "superio/WS2812.h"

static MODXO_TD_DRIVER_T *td_driver = NULL;

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

    modxo_ports_init(td_driver);
}

void modxo_poll_core1()
{
    modxo_ports_poll();
}

void modxo_poll_core0()
{
    if (td_driver)
        td_driver->poll();
        // Lpc log poll
#ifdef LPC_LOGGING
    lpc_interface_poll();
#endif
    ws2812_poll();
}

void modxo_lpc_reset()
{
    // Reset State Machines
    lpc_interface_start_sm();
}

void modxo_low_power_mode()
{
    // Modxo reset
    flashrom_set_mmc(MODXO_BANK_BOOTLOADER);
    ws2812_set_color(LedColorOff);

    // Modxo sleep
}

void modxo_init(MODXO_TD_DRIVER_T *drv)
{
    lpc_interface_init();

    modxo_lpcmem_init();

    if (drv && drv->command && drv->poll && drv->data)
        td_driver = drv;

    modxo_lpcio_init();
}