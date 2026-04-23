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
#include <string.h>

#include "FatFsSd.h"
//
#include "SerialUART.h"
#include "iostream/ArduinoStream.h"

// Serial output stream
ArduinoOutStream cout(Serial1);

/* Implement library message callbacks */
void put_out_error_message(const char *s) {
    Serial1.printf("%s\r", s);
}
void put_out_info_message(const char *s) {
    Serial1.printf("%s\r", s);
}
// This will not be called unless build_flags include "-D USE_DBG_PRINTF":
void put_out_debug_message(const char *s) {
    Serial1.printf("%s\r", s);
}

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define CHK_RESULT(s, fr)                                        \
    if (FR_OK != fr) {                                           \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

void setup() {
    Serial1.begin(115200);  // set up Serial library
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration

    /* This example assumes the following wiring:
    | GPIO | SPI1     | SD Card |
    | ---- | -------- | ------- |
    | GP8  | SPI1_RX  | D0/DO   |
    | GP10 | SPI1_SCK | CLK     |
    | GP11 | SPI1_TX  | CMD/DI  |
    | GP12 |          | D3/CS   |
    | GP14 |          | DET     |    
    */

    // GPIO numbers, not Pico pin numbers!

    /*
    Hardware Configuration of SPI "objects"
    Note: multiple SD cards can be driven by one SPI if they use different slave selects.
    Note: None, either or both of the RP2040 SPI components can be used.
    */

    // Hardware Configuration of SPI object:
    static spi_t spi = {
        .hw_inst = spi1,  // RP2040 SPI component
        .miso_gpio = 8,   // GPIO number (not Pico pin number)
        .mosi_gpio = 11,
        .sck_gpio = 10,
        .baud_rate = 12 * 1000 * 1000,  // Actual frequency: 10416666
        .set_drive_strength = true,
        .mosi_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA,
        .sck_gpio_drive_strength = GPIO_DRIVE_STRENGTH_12MA
    };

    // Hardware Configuration of SPI Interface object:
    static sd_spi_if_t spi_if = {
        .spi = &spi,   // Pointer to the SPI driving this card
        .ss_gpio = 12  // The SPI slave select GPIO for this SD card
    };

    // Hardware Configuration of the SD Card object:
    static sd_card_t sd_card = {
        .type = SD_IF_SPI,
        .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 14,  
        .card_detected_true = 0, // What the GPIO read returns when a card is present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true                                 
    };
    
    // static FatFsNs::SdCard card = FatFsNs::SdCard(&sd_card);
    FatFsNs::SdCard* card_p(FatFsNs::FatFs::add_sd_card(&sd_card));

    // The H/W config must be set up before this is called:
    sd_init_driver(); 

    /* ********************************************************************** */
    cout << "\033[2J\033[H";  // Clear Screen
    cout << "Hello, world!" << endl;
    FRESULT fr = card_p->mount();
    CHK_RESULT("mount", fr);
    FatFsNs::File file;
    char const* const filename = "filename.txt";
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    CHK_RESULT("open", fr);
    char const* const str = "Hello, world!\n";
    if (file.printf(str) < strlen(str)) {
        cout << "printf failed\n"
             << endl;
        for (;;) __breakpoint();
    }
    fr = file.close();
    CHK_RESULT("close", fr);
    fr = card_p->unmount();
    CHK_RESULT("unmount", fr);

    cout << "Goodbye, world!" << endl;
}
void loop() {}
