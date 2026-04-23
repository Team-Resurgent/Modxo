/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Hardware description for carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico, driven by
board pinout (see modxo_pinout.h). SDIO layout matches simple_sdio example:
CMD/D0 and contiguous D1..D3/CLK derived in sd_sdio_ctor.
*/

#include "hw_config.h"
#include "modxo_pinout.h"

#if SD_CARD_USE_SDIO

static sd_sdio_if_t sdio_if = {
    .CMD_gpio = SD_CARD_SDIO_CMD,
    .D0_gpio = SD_CARD_SDIO_D0,
    .SDIO_PIO = SD_CARD_SDIO_PIO,
    .DMA_IRQ_num = SD_CARD_SDIO_DMA_IRQ,
    .baud_rate = 266 * 1000 * 1000 / 12,
};

static sd_card_t sd_card = {
    .type = SD_IF_SDIO,
    .sdio_if_p = &sdio_if,
};

#elif SD_CARD_SPI_ENABLE

static spi_t spi = {
    .hw_inst = SD_CARD_SPI_INST,
    .sck_gpio = SD_CARD_SPI_CLK,
    .mosi_gpio = SD_CARD_SPI_MOSI,
    .miso_gpio = SD_CARD_SPI_MISO,
    .baud_rate = 266 * 1000 * 1000 / 12,
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = SD_CARD_SPI_CSN,
};

static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
    .use_card_detect = false,
};

#endif

size_t sd_get_num(void) {
#if SD_CARD_USE_SDIO || SD_CARD_SPI_ENABLE
    return 1;
#else
    return 0;
#endif
}

sd_card_t *sd_get_by_num(size_t num) {
#if SD_CARD_USE_SDIO || SD_CARD_SPI_ENABLE
    if (num == 0) {
        return &sd_card;
    }
#endif
    (void)num;
    return NULL;
}
