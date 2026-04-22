/* 
Copyright 2023 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use 
this file except in compliance with the License. You may obtain a copy of the 
License at

   http://www.apache.org/licenses/LICENSE-2.0 
Unless required by applicable law or agreed to in writing, software distributed 
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR 
CONDITIONS OF ANY KIND, either express or implied. See the License for the 
specific language governing permissions and limitations under the License.
*/

#include <stdlib.h>
//
#include "FatFsSd_C.h"
//
#include "SerialUART.h"

#define printf Serial1.printf
#define puts Serial1.println

/* Implement library message callbacks */
void put_out_error_message(const char *s) {
    Serial1.write(s);
}
void put_out_info_message(const char *s) {
    Serial1.write(s);
}
// This will not be called unless build_flags include "-D USE_DBG_PRINTF":
void put_out_debug_message(const char *s) {
    Serial1.write(s);
}

/* ********************************************************************** */

/* This example assumes the following wiring:
| GPIO | Function                               | SD Card | SPI0     |
| ---- | -------------------------------------- | ------- | -------- |
| GP2  | CLK/SCK                                | CLK     | SPI0_SCK |
| GP3  | CMD/MOSI (COPI or Peripheral's SDI)    | CMD/DI  | SPI0_TX  |
| GP4  | DATA 0/MISO (CIPO or Peripheral's SDO) | D0/DO   | SPI0_RX  |
| GP7  | DATA 3/CS                              | D3/CS   |          |
| GP9  | Card Detect                            | DET     |          |
*/

// Hardware Configuration of SPI "objects"
// Note: multiple SD cards can be driven by one SPI if they use different slave
// selects.
static spi_t spi = {
    .hw_inst = spi0,  // RP2040 SPI component
    .miso_gpio = 4,
    .mosi_gpio = 3,
    .sck_gpio = 2,    // GPIO number (not Pico pin number)
    .baud_rate = 12 * 1000 * 1000   // Actual frequency: 10416666.
};

static sd_spi_if_t spi_if = {
    .spi = &spi,  // Pointer to the SPI driving this card
    .ss_gpio = 7      // The SPI slave select GPIO for this SD card
} ;

// Hardware Configuration of the SD Card "objects"
static sd_card_t sd_card = {
    .type = SD_IF_SPI,
    .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
    .use_card_detect = true,
    .card_detect_gpio = 9,  
    .card_detected_true = 0, // What the GPIO read returns when a card is present.
    .card_detect_use_pull = true,
    .card_detect_pull_hi = true                                 

};
extern "C" size_t sd_get_num() { 
    return 1; 
}
extern "C" sd_card_t *sd_get_by_num(size_t num) {
    if (0 == num) {
        return &sd_card;
    } else {
        return NULL;
    }
}

/* ********************************************************************** */

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    time_init();

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    sd_card_t *pSD = sd_get_by_num(0);
    FRESULT fr = f_mount(&pSD->state.fatfs, "", 1);
    if (FR_OK != fr) {
        printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
        abort();
    }
    FIL fil;
    const char* const filename = "filename.txt";
    fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
        abort();
    }
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
        abort();
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        abort();
    }
    f_unmount("");

    puts("Goodbye, world!");
}
void loop() {}
