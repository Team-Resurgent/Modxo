
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

There should be one element of the spi[] array for each RP2040 hardware SPI used.

There should be one element of the spi_ifs[] array for each SPI interface object.
* Each element of spi_ifs[] must point to an spi_t instance with member "spi".

There should be one element of the sdio_ifs[] array for each SDIO interface object.

There should be one element of the sd_cards[] array for each SD card slot.
* Each element of sd_cards[] must point to its interface with spi_if_p or sdio_if_p.
*/

/* Hardware configuration for Pico SD Card Development Board
See https://oshwlab.com/carlk3/rp2040-sd-card-dev

See https://docs.google.com/spreadsheets/d/1BrzLWTyifongf_VQCc2IpJqXWtsrjmG7KnIbSBy-CPU/edit?usp=sharing,
tab "Dev Brd", for pin assignments assumed in this configuration file.
*/

#include <assert.h>
//
#include "my_debug.h"
//
#include "hw_config.h"

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects (or "chip selects").
static spi_t spis[] = {  // One for each RP2040 SPI component used
    {   // spis[0]
        .hw_inst = spi0,  // RP2040 SPI component
        .sck_gpio = 2,    // GPIO number (not Pico pin number)
        .mosi_gpio = 3,
        .miso_gpio = 4,
        /* GPIO reset drive strength is 4 mA.
         * When set_drive_strength is true,
         * the drive strength is set to 2 mA unless otherwise specified. */
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .no_miso_gpio_pull_up = true,
        // .baud_rate = 125 * 1000 * 1000 / 10 // 12500000 Hz
        // .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
        .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
        // .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    },
    {   // spis[1]
        .hw_inst = spi1,  // RP2040 SPI component
        .miso_gpio = 8,   // GPIO number (not Pico pin number)
        .sck_gpio = 10,
        .mosi_gpio = 11,
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .no_miso_gpio_pull_up = true,
        //.baud_rate = 125 * 1000 * 1000 / 12
        //.baud_rate = 125 * 1000 * 1000 / 10  // 12500000 Hz
        //.baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
        .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
        // .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    }
};

/* SPI Interfaces */
static sd_spi_if_t spi_ifs[] = {
    {   // spi_ifs[0]
        .spi = &spis[0],  // Pointer to the SPI driving this card
        .ss_gpio = 7,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
    },
    {   // spi_ifs[1]
        .spi = &spis[1],   // Pointer to the SPI driving this card
        .ss_gpio = 12,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
    },
    {   // spi_ifs[2]
        .spi = &spis[1],   // Pointer to the SPI driving this card
        .ss_gpio = 13,     // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA
    }
};

/* SDIO Interfaces */
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
static sd_sdio_if_t sdio_ifs[] = {
    {   // sdio_ifs[0]
        .CMD_gpio = 3,
        .D0_gpio = 4,
        /* GPIO reset drive strength is 4 mA.
         * When set_drive_strength is true,
         * the drive strength is set to 2 mA unless otherwise specified. */
        .set_drive_strength = true,
        .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .SDIO_PIO = pio1,
        .DMA_IRQ_num = DMA_IRQ_1,
        // .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
        // .baud_rate = 125 * 1000 * 1000 / 7  // 17857143 Hz
        // .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
        // .baud_rate = 125 * 1000 * 1000 / 5  // 25000000 Hz
        .baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    },
    {   // sdio_ifs[1]
        .CMD_gpio = 17,
        .D0_gpio = 18,
        .set_drive_strength = true,
        .CLK_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .CMD_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D0_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D1_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D2_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .D3_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA,
        .DMA_IRQ_num = DMA_IRQ_1,
        // .baud_rate = 125 * 1000 * 1000 / 8  // 15625000 Hz
        // .baud_rate = 125 * 1000 * 1000 / 7  // 17857143 Hz
        .baud_rate = 125 * 1000 * 1000 / 6  // 20833333 Hz
        // .baud_rate = 125 * 1000 * 1000 / 5  // 25000000 Hz
        //.baud_rate = 125 * 1000 * 1000 / 4  // 31250000 Hz
    }
};

/* Hardware Configuration of the SD Card "objects"
    These correspond to SD card sockets
*/
static sd_card_t sd_cards[] = {  // One for each SD card
#ifdef SPI_SD0
    {   // sd_cards[0]: Socket sd0
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[0],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 9,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
#else
    {   // sd_cards[0]: Socket sd0
        .type = SD_IF_SDIO,
        .sdio_if_p = &sdio_ifs[0],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 9,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
#endif
    {   // sd_cards[1]: Socket sd1
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[1],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 14,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
    {   // sd_cards[2]: Socket sd2
        .type = SD_IF_SPI,
        .spi_if_p = &spi_ifs[2],  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 15,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    },
    {   // sd_cards[3]: Socket sd3
        .type = SD_IF_SDIO,
        .sdio_if_p = &sdio_ifs[1], // Pointer to the interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 22,  
        .card_detected_true = 0, // What the GPIO read returns when a card is
                                 // present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true
    }
};

/* ********************************************************************** */

size_t sd_get_num() { return count_of(sd_cards); }

/**
 * @brief Get a pointer to an SD card object by its number.
 *
 * @param[in] num The number of the SD card to get.
 *
 * @return A pointer to the SD card object, or @c NULL if the number is invalid.
 */
sd_card_t *sd_get_by_num(size_t num) {
    assert(num < sd_get_num());
    if (num < sd_get_num()) {
        return &sd_cards[num];
    } else {
        return NULL;
    }
}

/* [] END OF FILE */
