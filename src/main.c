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
#include <string.h>
#include <pico/stdlib.h>
#include <pico/multicore.h>
#include <hardware/clocks.h>
#include <hardware/gpio.h>

#include <flashrom.h>
#include <lpc_mapper.h>
#include <modxo.h>
#include <modxo_pinout.h>
#include <ws2812.h>
#include <legacy_display.h>
#include <LPC47M152.h>
#include <uart_16550.h>
#include <modxo/lpc_interface.h>
#include <hardware/flash.h>

#if !PICO_RP2040
#include <hardware/structs/qmi.h>
#else
#include <hardware/structs/ssi.h>
#endif

// Modxo nvm contents
nvm_register_t* nvm_registers[] = {
    &ws2812_nvm,
};

uint8_t nvm_total_registers = sizeof(nvm_registers) / sizeof(nvm_registers[0]);
uint8_t detected_clkdiv = 0;

bool reset_pin = false;
bool xbox_active = false;

void modxo_init_interrupts_core1();

void core1_main()
{
    while (true)
    {
        if(xbox_active) {
            modxo_poll_core1();
        }
        __wfe();
    }
}

void core0_main()
{
    while (true)
    {
        __wfi();
    }
}

void core1_init() {
    run_modxo_handlers(mxt_fn_core1_init);
    modxo_init_interrupts_core1();

    core1_main(); // Infinite loop
}

void init_status_led() {
    gpio_init(LED_STATUS_PIN);
    gpio_set_dir(LED_STATUS_PIN, GPIO_OUT);
    gpio_put(LED_STATUS_PIN, LED_STATUS_ON_LEVEL);
}

void reset_pin_falling()
{
    modxo_lpc_reset_off();
}

void reset_pin_rising()
{
    modxo_lpc_reset_on();
}

void pin_3_3v_falling()
{
    xbox_active = false;
    modxo_shutdown();
    gpio_set_irq_enabled(LPC_ON, GPIO_IRQ_LEVEL_HIGH, true);
}

void pin_3_3v_high()
{
    set_sys_clock_khz(SYS_FREQ_IN_KHZ, true);
    gpio_set_irq_enabled(LPC_ON, GPIO_IRQ_LEVEL_HIGH, false);
    init_status_led();
    modxo_reset();
    xbox_active = true;
}

void core0_irq_handler(uint gpio, uint32_t event)
{
    if (gpio == LPC_ON && (event & GPIO_IRQ_EDGE_FALL) != 0)
    {
        pin_3_3v_falling();
    }

    // Use LEVEL_HIGH because rising edge has already passed by the time we get here
    if (gpio == LPC_ON && (event & GPIO_IRQ_LEVEL_HIGH) != 0)
    {
        pin_3_3v_high();
    }
}

void core1_irq_handler(uint gpio, uint32_t event)
{
    if (gpio == LPC_RESET && (event & GPIO_IRQ_EDGE_FALL) != 0)
    {
        reset_pin_falling();
    }

    if (gpio == LPC_RESET && (event & GPIO_IRQ_EDGE_RISE) != 0)
    {
        reset_pin_rising();
    }
}

void modxo_init_pin_irq(uint pin, uint32_t event)
{
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled(pin, event, true);
}

void modxo_init_interrupts()
{
    gpio_set_irq_callback(core0_irq_handler);
    modxo_init_pin_irq(LPC_ON, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_LEVEL_HIGH);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void modxo_init_interrupts_core1() {
    gpio_set_irq_callback(core1_irq_handler);
    modxo_init_pin_irq(LPC_RESET, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE);
    irq_set_enabled(IO_IRQ_BANK0, true);
}

void register_handlers()
{
    modxo_register_handler(&flashrom_hdlr);
    modxo_register_handler(&lpc_mapper_hdlr);
    modxo_register_handler(&lpc_interface_hdlr);
    modxo_register_handler(&LPC47M152_hdlr);
    modxo_register_handler(&uart_16550_hdlr);
    modxo_register_handler(&config_nvm_hdlr);
    modxo_register_handler(&ws2812_hdlr);
    modxo_register_handler(&legacy_display_hdlr);
}

#define RDID_BUF_SIZE 8
uint8_t get_flash_spi_clkdiv() {
    uint8_t txbuf[RDID_BUF_SIZE] = {0x9f}; // JEDEC ID command
    uint8_t rxbuf[RDID_BUF_SIZE] = {0};

    flash_do_cmd(txbuf, rxbuf, RDID_BUF_SIZE);

    // rxbuf[0] has garbage in it
    uint8_t manuf_id   = rxbuf[1];
    uint8_t device_id1 = rxbuf[2];
    uint8_t device_id2 = rxbuf[3];

    switch(manuf_id) {
    case 0x68: // BoHong/BYTe (Found on some cheap clone boards, missing SFDP table)
        if(device_id1 == 0x40 && device_id2 == 0x15) return 4; // BH25D16A/BY25D16AS (2MB, 108MHz)
        break;
    case 0x5E: // ZBit
        if(device_id1 == 0x40 && device_id2 == 0x18) return 2; // ZB25VQ128D (16MB, 133MHz)
        break;
    case 0xEF: // Winbond
        if(device_id1 == 0x40 && device_id2 == 0x15) return 2; // W25Q16JV (2MB, 133MHz)
        if(device_id1 == 0x40 && device_id2 == 0x16) return 2; // W25Q32JV (4MB, 133MHz)
        if(device_id1 == 0x40 && device_id2 == 0x18) return 2; // W25Q128JV (16MB, 133MHz)
        break;
    }

    return 4; // Default to CLKDIV=4, should be compatible with most flash chips, should be 66MHz SPI CLK
}

// Calling flash_do_cmd(), or anything that indirectly calls bootrom function flash_exit_xip(),
// will reset CLKDIV to 6, then restore to 4 by calling boot2 code (via flash_enable_xip_via_boot2())
// Currently, that's:
//  - flash_start_xip()
//  - flash_range_erase()
//  - flash_range_program()
//  - flash_do_cmd()
//  - flash_get_unique_id() - (indirectly calls flash_do_cmd())
// https://github.com/raspberrypi/pico-bootrom-rp2040/blob/ef22cd8ede5bc007f81d7f2416b48db90f313434/bootrom/program_flash_generic.c#L72
void reset_flash_speed() {
    if(!detected_clkdiv) return;

#if !PICO_RP2040
    qmi_hw->m[0].timing = (qmi_hw->m[0].timing & ~QMI_M0_TIMING_CLKDIV_BITS) | (detected_clkdiv << QMI_M0_TIMING_CLKDIV_LSB);

    volatile uint8_t *dummy_addr = (uint8_t*)XIP_NOCACHE_NOALLOC_BASE;
    uint8_t dummy_val = dummy_addr[0]; // Dummy read required when changing QMI timing
#else
    ssi_hw->ssienr = 0;
    ssi_hw->baudr = detected_clkdiv; // This is PICO_FLASH_SPI_CLKDIV
    ssi_hw->ssienr = 1;
#endif
}

void detect_and_upgrade_flash_speed() {
    detected_clkdiv = get_flash_spi_clkdiv();
    reset_flash_speed();
}

int main(void)
{
    detect_and_upgrade_flash_speed();

    set_sys_clock_khz(SYS_FREQ_IN_KHZ, true);
    stdio_init_all();

#ifdef START_DELAY
    sleep_ms(2000);
#endif

    register_handlers();

    multicore_reset_core1();
    multicore_launch_core1(core1_init);

    modxo_init();
    set_sys_clock_khz(SYS_FREQ_DEFAULT, true);
    modxo_init_interrupts();
    core0_main(); // Infinite loop
}
