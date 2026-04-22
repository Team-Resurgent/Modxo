
/* hw_config.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

/*
This file should be tailored to match the hardware design.

See
  https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

*/

/* Hardware configuration for Expansion Module Type A
See https://oshwlab.com/carlk3/rpi-pico-sd-card-expansion-module-1
*/

#include "hw_config.h"

/*
Pins CLK_gpio, D1_gpio, D2_gpio, and D3_gpio are at offsets from pin D0_gpio.
The offsets are determined by sd_driver\SDIO\rp2040_sdio.pio.
    CLK_gpio = (D0_gpio + SDIO_CLK_PIN_D0_OFFSET) % 32;
    As of this writing, SDIO_CLK_PIN_D0_OFFSET is 30,
        which is -2 in mod32 arithmetic, so:
    CLK_gpio = D0_gpio -2.
    D1_gpio = D0_gpio + 1;
    D2_gpio = D0_gpio + 2;
    D3_gpio = D0_gpio + 3;
*/

/* `baud_rate` setting:
The `baud_rate` is derived from the system core clock (`clk_sys`).
`sm_config_set_clkdiv` sets the state machine clock divider
in a PIO state machine configuration
from a floating point value we'll call "clk_div".

It is a fractional divider,
and the jitter introduced by a fractional divisor may be unacceptable.
See the datasheet for details.
The PIO state machine itself divides by `CLKDIV`,
defined in `sd_driver\SDIO\rp2040_sdio.pio`, currently 4.

  baud_rate = clk_sys / (CLKDIV * clk_div)

Preferrably, choose `baud_rate` for an interger clk_div.
For example, if clk_sys = 133MHz, and clk_div = 2.0:

  baud_rate = 133000000 / (4 * 2.0) = 16625000.

If clk_sys = 133MHz The maximum is:

  baud_rate = 133000000 / (4 * 1.0) = 33250000

You can solve for clk_div by:

  clk_div = clk_sys / (baud_rate * 4)

For example, if clk_sys = 150000000, and baud_rate = 25000000:

  clk_div = 150000000 / (25000000 * 4) = 1.5

*/

/* SDIO Interface */
static sd_sdio_if_t sdio_if = {
    .CMD_gpio = 3,
    .D0_gpio = 4,
    .set_drive_strength = true,
    .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
    .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
    .SDIO_PIO = pio1,
    .DMA_IRQ_num = DMA_IRQ_1,
#if PICO_RP2040
    // The default system clock frequency for SDK is 125MHz.
    .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
#endif
#if PICO_RP2350
    //◦The default system clock on RP2350 is 150Mhz.
    .baud_rate = 150 * 1000 * 1000 / 6  // 25000000 Hz, clk_div = 1.5
#endif
};

// Hardware Configuration of the SD Card socket "object"
static sd_card_t sd_card = {    
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
    // SD Card detect:
    .use_card_detect = true,
    .card_detect_gpio = 9,  
    .card_detected_true = 0  // What the GPIO read returns when a card is present.
};

/* ********************************************************************** */

/**
 * @brief Returns the number of sd_card_t objects that are available.
 * @return The number of sd_card_t objects.
 */
size_t sd_get_num(void) {
    return 1;
}

/**
 * @brief Return the sd_card_t object at the given index (0-based).
 * @param num The index of the sd_card_t object.
 * @return Pointer to the sd_card_t object at the given index if it exists, NULL otherwise.
 */
sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num)
        return &sd_card;
    else
        return NULL;
}

/* [] END OF FILE */
