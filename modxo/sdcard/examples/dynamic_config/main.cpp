/* Instead of a statically linked hw_config.c,
   create configuration dynamically.   
   
   This file should be tailored to match the hardware design.

    See 
    https://github.com/carlk3/no-OS-FatFS-SD-SDIO-SPI-RPi-Pico/tree/main#customizing-for-the-hardware-configuration

    */



#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "f_util.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "my_rtc.h"
//
#include "hw_config.h"

static std::vector<spi_t *> spis;             // SPI H/W components
static std::vector<sd_spi_if_t *> spi_ifs;    // SPI Interfaces
static std::vector<sd_sdio_if_t *> sdio_ifs;  // SDIO Interfaces
static std::vector<sd_card_t *> sd_cards;     // SD Card Sockets

size_t sd_get_num() { return sd_cards.size(); }

sd_card_t *sd_get_by_num(size_t num) {
    if (num <= sd_get_num()) {
        return sd_cards[num];
    } else {
        return NULL;
    }
}

static void test(sd_card_t *sd_card_p) {
    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    char const * const drive_prefix = sd_get_drive_prefix(sd_card_p);
    printf("Testing drive %s\n", drive_prefix);
    FRESULT fr = f_mount(&sd_card_p->state.fatfs, drive_prefix, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
    fr = f_chdrive(drive_prefix);
    if (FR_OK != fr) panic("f_chdrive error: %s (%d)\n", FRESULT_str(fr), fr);

    FIL fil;
    const char *const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
    }

    f_unmount(drive_prefix);
}

int main() {
    stdio_init_all();
    time_init();

    puts("Hello, world!");

    /* Hardware Configuration of SPI "objects" */

    // spis[0]
    spi_t *spi_p = new spi_t();
    assert(spi_p);
    spi_p->hw_inst = spi0;  // RP2040 SPI component
    spi_p->sck_gpio = 2;    // GPIO number (not Pico pin number)
    spi_p->mosi_gpio = 3;
    spi_p->miso_gpio = 4;
    spi_p->set_drive_strength = true;
    spi_p->mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
    spi_p->sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA;
    spi_p->baud_rate = 12 * 1000 * 1000;  // Actual frequency: 10416666
    spis.push_back(spi_p);

    // spis[1]
    spi_p = new spi_t();
    assert(spi_p);
    spi_p->hw_inst = spi1;  // RP2040 SPI component
    spi_p->miso_gpio = 8;   // GPIO number (not Pico pin number)
    spi_p->sck_gpio = 10;
    spi_p->mosi_gpio = 11;
    spi_p->set_drive_strength = true;
    spi_p->mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA;
    spi_p->sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_2MA;
    spi_p->baud_rate = 12 * 1000 * 1000;  // Actual frequency: 10416666spi_p->
    spis.push_back(spi_p);

    /* SPI Interfaces */

    // spi_ifs[0]
    sd_spi_if_t *spi_if_p = new sd_spi_if_t();
    assert(spi_if_p);

    spi_if_p->spi = spis[0];  // Pointer to the SPI driving this card
    spi_if_p->ss_gpio = 7;    // The SPI slave select GPIO for this SD card
    spi_ifs.push_back(spi_if_p);

    // spi_ifs[1]
    spi_if_p = new sd_spi_if_t();
    assert(spi_if_p);
    spi_if_p->spi = spis[1];  // Pointer to the SPI driving this card
    spi_if_p->ss_gpio = 12;   // The SPI slave select GPIO for this SD card
    spi_ifs.push_back(spi_if_p);

    // spi_ifs[2]
    spi_if_p = new sd_spi_if_t();
    assert(spi_if_p);
    spi_if_p->spi = spis[1];  // Pointer to the SPI driving this card
    spi_if_p->ss_gpio = 13;   // The SPI slave select GPIO for this SD card
    spi_ifs.push_back(spi_if_p);

    /* SDIO Interfaces */
    sd_sdio_if_t *sd_sdio_if_p = new sd_sdio_if_t();
    assert(sd_sdio_if_p);
    // sdio_ifs[0]
    sd_sdio_if_p->CMD_gpio = 17;
    sd_sdio_if_p->D0_gpio = 18;
    sd_sdio_if_p->SDIO_PIO = pio1;
    sd_sdio_if_p->DMA_IRQ_num = DMA_IRQ_1;
    sd_sdio_if_p->baud_rate = 15 * 1000 * 1000;  // 15 MHz
    sdio_ifs.push_back(sd_sdio_if_p);

    /* Hardware Configuration of the SD Card "objects" */

    // sd_cards[0]
    sd_card_t *sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SPI;
    sd_card_p->spi_if_p = spi_ifs[0];  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 9;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    // sd_cards[1]: Socket sd1
    sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SPI;
    sd_card_p->spi_if_p = spi_ifs[1];  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 14;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    // sd_cards[2]: Socket sd2
    sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SPI;
    sd_card_p->spi_if_p = spi_ifs[2];  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 15;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    // sd_cards[3]: Socket sd3
    sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SDIO;
    sd_card_p->sdio_if_p = sdio_ifs[0];
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 22;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;
    sd_cards.push_back(sd_card_p);

    // The H/W config must be set up before this is called:
    sd_init_driver();     

    for (size_t i = 0; i < sd_get_num(); ++i)
        test(sd_get_by_num(i));

    puts("Goodbye, world!");

    for (;;)
        ;
}
