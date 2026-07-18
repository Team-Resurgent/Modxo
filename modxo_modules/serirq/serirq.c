
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "serirq.pio.h"
#include "hardware/clocks.h"

#include <modxo/lpc_interface.h>
#include <modxo.h>
#include <modxo_pinout.h>

#define NUM_LCLKS_PER_IRQ_FRAME 3

PIO serirq_pio = pio1;
uint8_t serirq_pin = LPC_SERIRQ;
uint8_t serirq_sm = 2;
uint32_t serirq_offset = 0;

static void serirq_pio_init() {
    if(!pio_can_add_program(serirq_pio, &serirq_program))
        pio_remove_program(serirq_pio, &serirq_program, serirq_offset);

    serirq_offset = pio_add_program(serirq_pio, &serirq_program);

    pio_gpio_init(serirq_pio, serirq_pin);
    gpio_disable_pulls(serirq_pin);

    pio_sm_set_consecutive_pindirs(serirq_pio, serirq_sm, serirq_pin, 1, true);
    pio_sm_set_pins(serirq_pio, serirq_sm, 1);

    pio_sm_config c = serirq_program_get_default_config(serirq_offset);
    sm_config_set_jmp_pin (&c, serirq_pin);
    sm_config_set_set_pins(&c, serirq_pin, 1);
    sm_config_set_clkdiv(&c, 4); // 266mhz sysclk / 4 = 66.5mhz (LCLK freq * 2)

    pio_sm_init(serirq_pio, serirq_sm, serirq_offset, &c);
    pio_sm_set_enabled(serirq_pio, serirq_sm, true);
    pio_sm_exec(serirq_pio, serirq_sm, pio_encode_set(pio_pins, 1));
}

static void serirq_core1_init() {
    serirq_pio_init();
}

void serirq_trigger_irq(uint32_t irq) {
    // Xbox only supports 21 IRQs by default thru SERIRQ line
    if(irq > 20) return;

    serirq_pio->txf[serirq_sm] = irq * NUM_LCLKS_PER_IRQ_FRAME; // Number of LCLKs to wait
}

static void serirq_lpc_reset() {
    // Remove these vvvv after testing
    // serirq_trigger_irq(0);
    // serirq_trigger_irq(1);
    // serirq_trigger_irq(2);
    // serirq_trigger_irq(20);
}

static void serirq_init() {
    // lpc_interface_add_io_handler(WS2812_PORT_BASE, WS2812_ADDRESS_MASK, lpc_port_read, lpc_port_write);
    // lpc_interface_add_io_handler(MODXO_REGISTER_NVM_CONFIG_SEL, 0xFFFE, config_read_hdlr, config_write_hdlr);
}


MODXO_TASK serirq_hdlr = {
    .init = serirq_init,
    .core1_init = serirq_core1_init,
    .lpc_reset_on = serirq_lpc_reset
};
