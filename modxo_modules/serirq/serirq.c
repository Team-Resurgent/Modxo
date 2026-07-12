
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "serirq.pio.h"

#include <modxo/lpc_interface.h>
#include <modxo.h>
#include <modxo_pinout.h>

static void serirq_core1_init()
{
    uint offset = pio_add_program(pio1, &serirq_program);

    serirq_program_init(pio1, 3, offset, 1, 800000, 0);
}

static void serirq_init() {
    // lpc_interface_add_io_handler(WS2812_PORT_BASE, WS2812_ADDRESS_MASK, lpc_port_read, lpc_port_write);
    // lpc_interface_add_io_handler(MODXO_REGISTER_NVM_CONFIG_SEL, 0xFFFE, config_read_hdlr, config_write_hdlr);
}


MODXO_TASK serirq_hdlr = {
    .init = serirq_init,
    .core1_init = serirq_core1_init,
};
