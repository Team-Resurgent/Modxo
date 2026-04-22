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

#include <assert.h>
#include <stdlib.h>
#include <vector>
//
#include "FatFsSd.h"
//
#include "SerialUART.h"
#include "iostream/ArduinoStream.h"

// Serial output stream
ArduinoOutStream cout(Serial1);

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

void test(FatFsNs::SdCard* SdCard_p) {

    cout << "Testing drive " << SdCard_p->get_name() << endl;

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    FRESULT fr = SdCard_p->mount();
    if (FR_OK != fr) {
        cout << "mount error: " << FRESULT_str(fr) << " (" << fr << ")" << endl;
        abort();
    }
    fr = FatFsNs::FatFs::chdrive(SdCard_p->get_name());
    if (FR_OK != fr) {
        cout << "chdrive error: " << FRESULT_str(fr) << " (" << fr << ")" << endl;
        abort();
    }

    FatFsNs::File file;
    const char *const filename = "filename.txt";
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        cout << "open(" << filename << ") error: " << FRESULT_str(fr) << " (" << fr << ")" << endl;
        abort();
    }
    if (file.printf("Hello, world!\n") < 0) {
        cout << "printf failed" << endl;
        abort();
    }
    fr = file.close();
    if (FR_OK != fr) {
        cout << "close error: " << FRESULT_str(fr) << " (" << fr << ")" << endl;
        abort();
    }
    SdCard_p->unmount();
}

/* ********************************************************************** */
/*
This example assumes the following wiring for SD card 0:
    | GPIO | Function                         | SD Card | SPI0     |
    | ---- | -------------------------------- | ------- | -------- |
    | GP2  | SCK                              | CLK     | SPI0_SCK |
    | GP3  | MOSI (COPI or Peripheral's SDI)  | CMD/DI  | SPI0_TX  |
    | GP4  | MISO (CIPO or Peripheral's SDO)  | D0/DO   | SPI0_RX  |
    | GP7  | SS (CS)                          | D3/CS   |          |
    | GP9  | Card Detect                      | DET     |          |

This example assumes the following wiring for SD card 1:
    | GPIO | SD Card |
    | ---- | ------- |
    | GP16 | CLK     |
    | GP17 | CMD     |
    | GP18 | D0      |
    | GP19 | D1      |
    | GP20 | D2      |
    | GP21 | D3      |
    | GP22 | DET     |
*/

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    cout << "\033[2J\033[H";  // Clear Screen
    cout << "Hello, world!" << endl;

    pinMode(LED_BUILTIN, OUTPUT);
    time_init();

    // Hardware Configuration of SPI "object"
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

    // Hardware Configurtion of the SPI Interface "object"
    sd_spi_if_t *spi_if_p = new sd_spi_if_t();
    assert(spi_if_p);
    spi_if_p->spi = spi_p;  // Pointer to the SPI driving this card
    spi_if_p->ss_gpio = 7;    // The SPI slave select GPIO for this SD card

    // Hardware Configuration of the SD Card "object"
    sd_card_t *sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SPI;
    sd_card_p->spi_if_p = spi_if_p;  // Pointer to the SPI interface driving this card
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 9;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;

    FatFsNs::FatFs::add_sd_card(sd_card_p);

    // Hardware Configurtion of the SDIO Interface "object"
    sd_sdio_if_t *sd_sdio_if_p = new sd_sdio_if_t();
    assert(sd_sdio_if_p);
    sd_sdio_if_p->CMD_gpio = 17,
    sd_sdio_if_p->D0_gpio = 18,
    sd_sdio_if_p->SDIO_PIO = pio1,
    sd_sdio_if_p->DMA_IRQ_num = DMA_IRQ_1,
    sd_sdio_if_p->baud_rate = 15 * 1000 * 1000;  // 15 MHz

    // Hardware Configuration of the SD Card "object"
    sd_card_p = new sd_card_t();
    assert(sd_card_p);
    sd_card_p->type = SD_IF_SDIO;
    sd_card_p->sdio_if_p = sd_sdio_if_p;
    sd_card_p->use_card_detect = true;
    sd_card_p->card_detect_gpio = 22;
    sd_card_p->card_detected_true = 0;  // What the GPIO read returns when a card is present
    sd_card_p->card_detect_use_pull = true;
    sd_card_p->card_detect_pull_hi = true;

    FatFsNs::FatFs::add_sd_card(sd_card_p);

    // The H/W config must be set up before this is called:
    sd_init_driver(); 

    for (size_t i = 0; i < FatFsNs::FatFs::SdCard_get_num(); ++i)
        test(FatFsNs::FatFs::SdCard_get_by_num(i));

    cout << "Goodbye, world!" << endl;
}
void loop() {
    while (true) {
        digitalWrite(LED_BUILTIN, HIGH);
        sleep_ms(250);
        digitalWrite(LED_BUILTIN, LOW);  
        sleep_ms(250);
    }
}
