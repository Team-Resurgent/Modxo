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
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "hardware/structs/padsbank0.h"
#include "hardware/structs/bus_ctrl.h"
#include "lpc_interface.h"
#include "modxo_pinout.h"

#include "lpc_comm.pio.h"
#include "lpc_log.h"

void start_mem_read_sm(void);

typedef struct
{
    uint32_t nibbles_read;
    lpc_handler_cback handler;
    uint8_t cyctype_dir;
    uint8_t address_len;
} LPC_SM_HANDLER;

typedef struct
{
    uint16_t port_base;
    uint16_t mask;
    SUPERIO_PORT_CALLBACK_T read_cback;
    SUPERIO_PORT_CALLBACK_T write_cback;
} SUPERIO_PORT_HANDLER_T;

static void io_write_hdlr(uint32_t address, uint8_t *data);
static void io_read_hdlr(uint32_t address, uint8_t *data);

LPC_SM_HANDLER lpc_handlers[LPC_OP_TOTAL] = {
    [LPC_OP_IO_READ] = {.nibbles_read = 4, .cyctype_dir = 0, .handler = io_read_hdlr, .address_len = 16},
    [LPC_OP_IO_WRITE] = {.nibbles_read = 6, .cyctype_dir = 2, .handler = io_write_hdlr, .address_len = 16},
    [LPC_OP_MEM_READ] = {.nibbles_read = 8, .cyctype_dir = 4, .handler = NULL, .address_len = 32},
    [LPC_OP_MEM_WRITE] = {.nibbles_read = 10, .cyctype_dir = 6, .handler = NULL, .address_len = 32},
}; // 4 SM per PIO

static const char *LPC_OP_STRINGS[LPC_OP_TOTAL] = {
    [LPC_OP_IO_READ] = "IO_READ  ",
    [LPC_OP_IO_WRITE] = "IO_WRITE ",
    [LPC_OP_MEM_WRITE] = "MEM_WRITE",
    [LPC_OP_MEM_READ] = "MEM_READ ",
};

#define SUPERIO_HANDLER_MAX_ENTRIES 32
static SUPERIO_PORT_HANDLER_T hdlr_table[SUPERIO_HANDLER_MAX_ENTRIES];
static uint8_t total_entries = 0;

static void io_write_hdlr(uint32_t address, uint8_t *data)
{
    for (uint8_t tidx = 0; tidx < total_entries; tidx++)
    {
        if ((address & hdlr_table[tidx].mask) == hdlr_table[tidx].port_base)
        {
            hdlr_table[tidx].write_cback(address, data);
        }
    }
}

static void io_read_hdlr(uint32_t address, uint8_t *data)
{
    for (uint8_t tidx = 0; tidx < total_entries; tidx++)
    {
        if ((address & hdlr_table[tidx].mask) == hdlr_table[tidx].port_base)
        {
            hdlr_table[tidx].read_cback(address, data);
        }
    }
}

// PIO
static PIO _pio;
static bool _disable_internal_flash = true;
static uint offset;

static void lpc_gpio_init(PIO pio)
{
    // Connect the GPIOs to selected PIO block
    for (uint i = LPC_LAD_START; i < LPC_LAD_START + LAD_PIN_COUNT; i++)
    {
        pio_gpio_init(pio, i);
        gpio_disable_pulls(i);
    }

    pio_gpio_init(pio, LPC_CLK);
    gpio_disable_pulls(LPC_CLK);

    pio_gpio_init(pio, LPC_LFRAME);
    gpio_disable_pulls(LPC_LFRAME);
}

static void gpio_set_max_drivestrength(io_rw_32 gpio, uint32_t strength)
{
    hw_write_masked(
        &padsbank0_hw->io[gpio],
        strength << PADS_BANK0_GPIO0_DRIVE_LSB,
        PADS_BANK0_GPIO0_DRIVE_BITS);
}

static void pio_custom_init(PIO pio, uint sm, uint offset, bool lframe_cancel)
{
    lpc_read_request_init(pio, sm, offset, lpc_handlers[sm].address_len, lframe_cancel);

    pio->txf[sm] = lpc_handlers[sm].cyctype_dir;
    pio->txf[sm] = lpc_handlers[sm].nibbles_read - 1;

    pio_sm_exec(pio, sm, pio_encode_mov(pio_pins, pio_null));
    pio_sm_exec(pio, sm, pio_encode_pull(false, true));
    pio_sm_exec(pio, sm, pio_encode_mov(pio_y, pio_osr));
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
}

static void lpc_read_handler(uint8_t sm)
{
    register uint32_t address, shifted, pushed;
    uint8_t result_data;
    address = _pio->rxf[sm];
    pushed = _pio->rxf[sm];

    if (lpc_handlers[sm].handler)
        lpc_handlers[sm].handler(address, &result_data);

    shifted = 0xF000;
    shifted |= (result_data << 4);
    _pio->txf[sm] = shifted;
    _pio->txf[sm] = lpc_handlers[sm].nibbles_read - 1;

#ifdef LPC_LOGGING
    {
        log_entry item;
        item.address = address;
        item.cyc_type = sm;
        item.data = result_data;
        lpclog_enqueue(item);
    }
#endif

    pio_interrupt_clear(_pio, sm);
}

static void lpc_write_handler(uint8_t sm)
{
    register uint32_t address, shifted, swaped_value;

    uint8_t result_data;
    address = _pio->rxf[sm];
    result_data = (uint8_t)_pio->rxf[sm];
    swaped_value = (result_data & 0xF) << 4;
    swaped_value |= result_data >> 4;
    result_data = (uint8_t)swaped_value;

    if (lpc_handlers[sm].handler)
        lpc_handlers[sm].handler(address, &result_data);

    shifted = 0xFFF0;
    _pio->txf[sm] = shifted;
    _pio->txf[sm] = lpc_handlers[sm].nibbles_read - 1;

#ifdef LPC_LOGGING
    {
        log_entry item;
        item.address = address;
        item.cyc_type = sm;
        item.data = result_data;
        lpclog_enqueue(item);
    }
#endif

    pio_interrupt_clear(_pio, sm);
}

static void lpc_request_handler(void)
{
    if (pio_interrupt_get(_pio, LPC_OP_MEM_READ))
    {
        lpc_read_handler(LPC_OP_MEM_READ);
    }
    else if (pio_interrupt_get(_pio, LPC_OP_MEM_WRITE))
    {
        lpc_write_handler(LPC_OP_MEM_WRITE);
    }
    else if (pio_interrupt_get(_pio, LPC_OP_IO_READ))
    {
        lpc_read_handler(LPC_OP_IO_READ);
    }
    else if (pio_interrupt_get(_pio, LPC_OP_IO_WRITE))
    {
        lpc_write_handler(LPC_OP_IO_WRITE);
    }
}

static void enable_pio_interrupts(void)
{
    pio_set_irq0_source_enabled(_pio, pis_interrupt0, true);
    pio_set_irq0_source_enabled(_pio, pis_interrupt1, true);
    pio_set_irq0_source_enabled(_pio, pis_interrupt2, true);
    pio_set_irq0_source_enabled(_pio, pis_interrupt3, true);
    irq_set_exclusive_handler(PIO0_IRQ_0, lpc_request_handler);
    irq_set_enabled(PIO0_IRQ_0, true);
    // irq_set_exclusive_handler(PIO0_IRQ_1, lpc_read_irq_handler);
    // irq_set_enabled(PIO0_IRQ_1, true);
}

/*
    Used for LFRAME Cancel
*/
void lpc_interface_disable_onboard_flash(bool disable)
{
    _disable_internal_flash = disable;
    if (_disable_internal_flash)
    {
        gpio_put(GPIO_D0, 0);
        gpio_set_dir(GPIO_D0, GPIO_OUT);
    }
    else
    {
        gpio_set_dir(GPIO_D0, GPIO_IN);
    }
}

void lpc_interface_set_callback(LPC_OP_TYPE op, lpc_handler_cback cback)
{
    lpc_handlers[op].handler = cback;
}

void lpc_interface_start_sm()
{
    pio_set_sm_mask_enabled(_pio, 15, false); // Disable All State Machines
    pio_custom_init(_pio, LPC_OP_MEM_READ, offset, _disable_internal_flash);
    pio_custom_init(_pio, LPC_OP_MEM_WRITE, offset, _disable_internal_flash);
    pio_custom_init(_pio, LPC_OP_IO_READ, offset, false);
    pio_custom_init(_pio, LPC_OP_IO_WRITE, offset, false);
    // Enable State Machines
    pio_sm_set_enabled(_pio, LPC_OP_MEM_READ, true);
    pio_sm_set_enabled(_pio, LPC_OP_MEM_WRITE, true);
    pio_sm_set_enabled(_pio, LPC_OP_IO_READ, true);
    pio_sm_set_enabled(_pio, LPC_OP_IO_WRITE, true);
    // pio_set_sm_mask_enabled(_pio, 15, true);//Enable All State Machines

    enable_pio_interrupts();
}

void lpc_interface_init()
{

    _pio = pio0;

    pio_claim_sm_mask(_pio, 15);

    if (pio_can_add_program(_pio, &lpc_read_request_program))
    {
        offset = pio_add_program(_pio, &lpc_read_request_program);
    }
    else
    {
        while (true)
        {
            gpio_put(LED_STATUS_PIN, 1);
            sleep_ms(250);
            printf("Error: pio program can not be loaded\n");
            gpio_put(LED_STATUS_PIN, 0);
            sleep_ms(250);
        }
    }

    lpc_gpio_init(_pio);

    gpio_init(GPIO_D0);
    gpio_disable_pulls(GPIO_D0);

    if (_disable_internal_flash)
    {
        gpio_put(GPIO_D0, 0);
        gpio_set_dir(GPIO_D0, GPIO_OUT);
    }
    else
    {
        gpio_set_dir(GPIO_D0, GPIO_IN);
    }

    // lpc_disable_tsop(disable_internal_flash);

    gpio_set_max_drivestrength(LPC_LFRAME, PADS_BANK0_GPIO0_DRIVE_VALUE_12MA);
    gpio_set_max_drivestrength(GPIO_D0, PADS_BANK0_GPIO0_DRIVE_VALUE_12MA);

    lpc_interface_start_sm();
}

bool lpc_interface_add_io_handler(uint16_t port_base, uint16_t mask, SUPERIO_PORT_CALLBACK_T read_cback, SUPERIO_PORT_CALLBACK_T write_cback)
{
    if (total_entries >= SUPERIO_HANDLER_MAX_ENTRIES)
        return false;

    hdlr_table[total_entries].port_base = port_base;
    hdlr_table[total_entries].mask = mask;
    hdlr_table[total_entries].read_cback = read_cback;
    hdlr_table[total_entries].write_cback = write_cback;

    total_entries++;
}

void lpc_interface_poll()
{
    log_entry entry;
    while (lpclog_dequeue(&entry))
    {
        if (entry.cyc_type == LPC_OP_IO_READ || entry.cyc_type == LPC_OP_IO_WRITE)
        {
            printf("\nLPC_LOG: %s [0x%04X] data:[%02X]", LPC_OP_STRINGS[entry.cyc_type], entry.address, entry.data);
        }
        else
        {
            printf("\nLPC_LOG: %s [0x%08X] data:[%02X]", LPC_OP_STRINGS[entry.cyc_type], entry.address, entry.data);
        }
    }
}