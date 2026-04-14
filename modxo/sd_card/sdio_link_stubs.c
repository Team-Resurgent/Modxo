/*
 * SPDX short identifier: BSD-2-Clause
 * Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
 *
 * Modxo SPI-only link stubs: upstream sd_card.c references SDIO entry points.
 * SDIO object files are not linked (no PIO). These satisfy the linker; they are
 * never called when all cards use SD_IF_SPI (see hw_config.c).
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "sd_card.h"
#include "SDIO/SdioCard.h"

void sd_sdio_ctor(sd_card_t *sd_card_p)
{
    (void)sd_card_p;
}

bool rp2040_sdio_get_sd_status(sd_card_t *sd_card_p, uint8_t response[64])
{
    (void)sd_card_p;
    if (response) {
        for (size_t i = 0; i < 64; i++) {
            response[i] = 0;
        }
    }
    return false;
}
