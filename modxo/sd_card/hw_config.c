/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Hardware mapping for no-OS-FatFS. Pins from modxo_pinout: SPI (default) or SDIO.
*/

#include <modxo_pinout.h>

#include "hw_config.h"

#if SD_CARD_USE_SDIO

#include <hardware/dma.h>
#include <hardware/pio.h>

#include "rp2040_sdio.pio.h"

static sd_sdio_if_t sdio_if = {
    .CLK_gpio = (SD_CARD_SDIO_D0 + SDIO_CLK_PIN_D0_OFFSET) % 32,
    .CMD_gpio = SD_CARD_SDIO_CMD,
    .D0_gpio = SD_CARD_SDIO_D0,
    .D1_gpio = SD_CARD_SDIO_D0 + 1,
    .D2_gpio = SD_CARD_SDIO_D0 + 2,
    .D3_gpio = SD_CARD_SDIO_D0 + 3,
    .SDIO_PIO = pio2,
    .DMA_IRQ_num = SD_CARD_SDIO_DMA_IRQ ? DMA_IRQ_1 : DMA_IRQ_0,
    .use_exclusive_DMA_IRQ_handler = false,
    .baud_rate = SD_CARD_SDIO_BAUD_RATE,
    .set_drive_strength = false,
};

static sd_card_t sd_card = {
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
};

#else /* SPI */

#include "SPI/my_spi.h"

static spi_t spi = {
    .hw_inst = SD_CARD_SPI_INST,
    .sck_gpio = SD_CARD_SPI_CLK,
    .mosi_gpio = SD_CARD_SPI_MOSI,
    .miso_gpio = SD_CARD_SPI_MISO,
    .baud_rate = 60 * 1000 * 1000,
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = SD_CARD_SPI_CSN,
};

static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
};

#endif

size_t sd_get_num(void)
{
    return 1;
}

sd_card_t *sd_get_by_num(size_t num)
{
    if (num == 0)
    {
        return &sd_card;
    }
    return NULL;
}
