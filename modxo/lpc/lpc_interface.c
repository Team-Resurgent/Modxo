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
#include "hardware/irq.h"
#include "hardware/structs/padsbank0.h"
#include "hardware/structs/bus_ctrl.h"
#include <modxo/lpc_interface.h>
#include <modxo_pinout.h>

#include <lpc_comm.pio.h>
#include <modxo/lpc_log.h>
#include <tusb.h>

typedef void (*lpc_handler)(uint32_t address, uint8_t * data);

typedef struct
{
    uint32_t nibbles_read;
    lpc_handler handler;
    uint8_t cyctype_dir;
    uint8_t address_len;
} LPC_SM_HANDLER;

typedef struct
{
    uint16_t addr_start;
    uint16_t addr_end;
    lpc_io_handler_cback read_cback;
    lpc_io_handler_cback write_cback;
} lpc_io_handler;

typedef struct
{
    uint8_t * read_buffer;
    uint32_t read_mask;
    uint8_t * write_buffer;
    uint32_t write_mask;
} lpc_mem_handler;

// Give something for empty lpc_mem_handler's to point to
uint8_t mem_read_dummy, mem_write_dummy;

static void io_read_dummy(uint16_t address, uint8_t * data) { *data = 0; }
static void io_write_dummy(uint16_t address, uint8_t * data) {}

static void mem_read_hdlr(uint32_t address, uint8_t * data);
static void mem_write_hdlr(uint32_t address, uint8_t * data);
static void io_read_hdlr(uint32_t address, uint8_t * data);
static void io_write_hdlr(uint32_t address, uint8_t * data);
static void gpio_set_max_drivestrength(io_rw_32 gpio, uint32_t strength);

LPC_SM_HANDLER lpc_handlers[LPC_OP_TOTAL] = {
    [LPC_OP_IO_READ] = {.nibbles_read = 4, .cyctype_dir = 0, .handler = io_read_hdlr, .address_len = 16},
    [LPC_OP_IO_WRITE] = {.nibbles_read = 6, .cyctype_dir = 2, .handler = io_write_hdlr, .address_len = 16},
    [LPC_OP_MEM_READ] = {.nibbles_read = 8, .cyctype_dir = 4, .handler = mem_read_hdlr, .address_len = 32},
    [LPC_OP_MEM_WRITE] = {.nibbles_read = 10, .cyctype_dir = 6, .handler = mem_write_hdlr, .address_len = 32},
}; // 4 SM per PIO

static const char *LPC_OP_STRINGS[LPC_OP_TOTAL] = {
    [LPC_OP_IO_READ] = "IO_READ  ",
    [LPC_OP_IO_WRITE] = "IO_WRITE ",
    [LPC_OP_MEM_WRITE] = "MEM_WRITE",
    [LPC_OP_MEM_READ] = "MEM_READ ",
};

#define IO_HANDLER_MAX_ENTRIES 32
static lpc_io_handler io_hdlr_table[IO_HANDLER_MAX_ENTRIES];
static uint8_t io_hdlr_count = 0;
static uint8_t io_read_hdlr_last = 0;
static uint8_t io_write_hdlr_last = 0;

static void io_read_hdlr(uint32_t address, uint8_t *data)
{
    if (!io_hdlr_count) return;

    // Start checking from the last handler accessed since subsequent read/writes are likely to be the same handler
    uint8_t tidx = io_read_hdlr_last;
    do
    {
        if (address >= io_hdlr_table[tidx].addr_start && address <= io_hdlr_table[tidx].addr_end)
        {
            io_hdlr_table[tidx].read_cback(address, data);

            io_read_hdlr_last = tidx;
            return;
        }

        tidx++;
        if (tidx >= io_hdlr_count) tidx = 0;
    }
    while(tidx != io_read_hdlr_last);
}

static void io_write_hdlr(uint32_t address, uint8_t *data)
{
    if (!io_hdlr_count) return;

    // Start checking from the last handler accessed since subsequent read/writes are likely to be the same handler
    uint8_t tidx = io_write_hdlr_last;
    do
    {
        if (address >= io_hdlr_table[tidx].addr_start && address <= io_hdlr_table[tidx].addr_end)
        {
            io_hdlr_table[tidx].write_cback(address, data);

            io_write_hdlr_last = tidx;
            return;
        }

        tidx++;
        if (tidx >= io_hdlr_count) tidx = 0;
    }
    while(tidx != io_write_hdlr_last);
}

// Decode one of 16 possible memory handlers using the following 'X' nibble: 0xFFX00000
#define MEM_HANDLER_COUNT 16
static lpc_mem_handler mem_hdlr_table[MEM_HANDLER_COUNT];

static inline lpc_mem_handler * mem_get_hdlr(uint32_t address)
{
    return mem_hdlr_table + ((address >> 20) & 0xF);
}

static void mem_read_hdlr(uint32_t address, uint8_t * data)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    *data = hdlr->read_buffer[address & hdlr->read_mask];
}

static void mem_write_hdlr(uint32_t address, uint8_t * data)
{
    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    hdlr->write_buffer[address & hdlr->write_mask] = *data;
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

    gpio_disable_pulls(LPC_LFRAME);
    gpio_set_oeover(LPC_LFRAME, 1);
    gpio_set_outover(LPC_LFRAME, 2);
    pio_gpio_init(pio, LPC_LFRAME);
}

static void gpio_set_max_drivestrength(io_rw_32 gpio, uint32_t strength)
{
    hw_write_masked(
        &padsbank0_hw->io[gpio],
        strength << PADS_BANK0_GPIO0_DRIVE_LSB,
        PADS_BANK0_GPIO0_DRIVE_BITS);
}

static void pio_custom_init(PIO pio, LPC_OP_TYPE sm, uint offset, bool lframe_cancel)
{
    lpc_read_request_init(pio, sm, offset, lpc_handlers[sm].address_len, lframe_cancel);

    pio->txf[sm] = lpc_handlers[sm].cyctype_dir;
    pio->txf[sm] = lpc_handlers[sm].nibbles_read - 1;

    pio_sm_exec(pio, sm, pio_encode_mov(pio_pins, pio_null));
    pio_sm_exec(pio, sm, pio_encode_pull(false, true));
    pio_sm_exec(pio, sm, pio_encode_mov(pio_y, pio_osr));
    pio_sm_exec(pio, sm, pio_encode_jmp(offset));
}

static void lpc_read_handler(LPC_OP_TYPE sm)
{
    register uint32_t address, shifted, pushed;
    uint8_t result_data;
    address = _pio->rxf[sm];
    pushed = _pio->rxf[sm];

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

static void lpc_write_handler(LPC_OP_TYPE sm)
{
    register uint32_t address, shifted, swaped_value;

    uint8_t result_data;
    address = _pio->rxf[sm];
    result_data = (uint8_t)_pio->rxf[sm];
    swaped_value = (result_data & 0xF) << 4;
    swaped_value |= result_data >> 4;
    result_data = (uint8_t)swaped_value;

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

void lpc_interface_start_sm(void)
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

void lpc_interface_reset(void)
{
    lpc_interface_start_sm();
}

void lpc_interface_init(void)
{
    lpc_interface_mem_global_read_handler(NULL, 0);
    lpc_interface_mem_global_write_handler(NULL, 0);

    _pio = pio0;

    pio_claim_sm_mask(_pio, 15);

    if(!pio_can_add_program(_pio, &lpc_read_request_program))
        pio_remove_program(_pio, &lpc_read_request_program, offset);

    offset = pio_add_program(_pio, &lpc_read_request_program);

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

int lpc_interface_io_add_handler(uint16_t addr_start, uint16_t addr_end, lpc_io_handler_cback read_cback, lpc_io_handler_cback write_cback)
{
    if (io_hdlr_count >= IO_HANDLER_MAX_ENTRIES) return -1;
    if (addr_end > addr_start) return -1;

    io_hdlr_table[io_hdlr_count].addr_start = addr_start;
    io_hdlr_table[io_hdlr_count].addr_end = addr_end;
    io_hdlr_table[io_hdlr_count].read_cback = read_cback != NULL ? read_cback : io_read_dummy;
    io_hdlr_table[io_hdlr_count].write_cback = write_cback != NULL ? write_cback : io_write_dummy;

    return io_hdlr_count++;
}

bool lpc_interface_io_set_addr(unsigned int hdlr_idx, uint16_t addr_start, uint16_t addr_end)
{
    if (hdlr_idx >= io_hdlr_count) return false;
    if (addr_end > addr_start) return false;

    io_hdlr_table[hdlr_idx].addr_start = addr_start;
    io_hdlr_table[hdlr_idx].addr_end = addr_end;

    return true;
}

void lpc_interface_mem_global_read_handler(uint8_t * read_buffer, uint32_t read_mask)
{
    if (read_buffer != NULL)
    {
        for (uint i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            mem_hdlr_table[i].read_buffer = read_buffer;
            mem_hdlr_table[i].read_mask = read_mask & 0x00FFFFFF;
        }
    }
    else
    {
        for (uint i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            // Point to the dummy buffer to avoid memory errors
            mem_hdlr_table[i].read_buffer = &mem_read_dummy;
            mem_hdlr_table[i].read_mask = 0;
        }
    }
}

void lpc_interface_mem_global_write_handler(uint8_t * write_buffer, uint32_t write_mask)
{
    if (write_buffer != NULL)
    {
        for (uint i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            mem_hdlr_table[i].write_buffer = write_buffer;
            mem_hdlr_table[i].write_mask = write_mask & 0x00FFFFFF;
        }
    }
    else
    {
        for (uint i = 0; i < MEM_HANDLER_COUNT; i++)
        {
            // Point to the dummy buffer to avoid memory errors
            mem_hdlr_table[i].write_buffer = &mem_write_dummy;
            mem_hdlr_table[i].write_mask = 0;
        }
    }
}

bool lpc_interface_mem_set_read_handler(uint32_t address, uint8_t * read_buffer, uint32_t read_mask)
{
    if ((address & 0xFF0FFFFF) != 0xFF000000) return false;

    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    if (read_buffer != NULL)
    {
        hdlr->read_buffer = read_buffer;
        hdlr->read_mask = read_mask & 0x00FFFFFF;
    }
    else
    {
        hdlr->read_buffer = &mem_read_dummy;
        hdlr->read_mask = 0;
    }

    return true;
}

bool lpc_interface_mem_set_write_handler(uint32_t address, uint8_t * write_buffer, uint32_t write_mask)
{
    if ((address & 0xFF0FFFFF) != 0xFF000000) return false;

    lpc_mem_handler * hdlr = mem_get_hdlr(address);
    if (write_buffer != NULL)
    {
        hdlr->write_buffer = write_buffer;
        hdlr->write_mask = write_mask & 0x00FFFFFF;
    }
    else
    {
        hdlr->write_buffer = &mem_write_dummy;
        hdlr->write_mask = 0;
    }

    return true;
}

static void lpc_interface_poll(void)
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

MODXO_TASK lpc_interface_hdlr = {
    .init = lpc_interface_init,
    .core0_poll = lpc_interface_poll
};
