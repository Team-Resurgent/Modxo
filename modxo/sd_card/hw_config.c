/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Hardware mapping for no-OS-FatFS (SPI). Pins come from modxo_pinout (SD_CARD_SPI_*).
*/

#include <modxo_pinout.h>

#include "hw_config.h"

static spi_t spi = {
    .hw_inst = SD_CARD_SPI_INST,
    .sck_gpio = SD_CARD_SPI_CLK,
    .mosi_gpio = SD_CARD_SPI_MOSI,
    .miso_gpio = SD_CARD_SPI_MISO,
    .baud_rate = 125 * 1000 * 1000 / 4,
};

static sd_spi_if_t spi_if = {
    .spi = &spi,
    .ss_gpio = SD_CARD_SPI_CSN,
};

static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,
};

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
