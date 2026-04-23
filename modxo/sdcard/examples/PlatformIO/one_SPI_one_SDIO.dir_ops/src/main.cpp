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

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;                     // Serial is via USB; wait for enumeration
    cout << "\033[2J\033[H";  // Clear Screen
    cout << "Hello, world!" << endl;

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

    /* Hardware Configuration of SPI object */
    static spi_t spi = {
        .hw_inst = spi0,  // RP2040 SPI component
        .miso_gpio = 4,
        .mosi_gpio = 3,
        .sck_gpio = 2,                 // GPIO number (not Pico pin number)
        .baud_rate = 12 * 1000 * 1000  // Actual frequency: 10416666.
    };

    static sd_spi_if_t spi_if = {
        .spi = &spi,  // Pointer to the SPI driving this card
        .ss_gpio = 7  // The SPI slave select GPIO for this SD card
    };

    static sd_sdio_if_t sdio_if = {
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
        .CMD_gpio = 17,
        .D0_gpio = 18,
        .SDIO_PIO = pio1,
        .DMA_IRQ_num = DMA_IRQ_1,
        .baud_rate = 15 * 1000 * 1000  // 15 MHz
    };

    // Hardware Configuration of the SD Card "objects"
    static sd_card_t sd_cards[] = {
        {   // sd_cards[0]
            .type = SD_IF_SPI,
            .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
            .use_card_detect = true,
            .card_detect_gpio = 9,
            .card_detected_true = 0,  // What the GPIO read returns when a card is present.
            .card_detect_use_pull = true,
            .card_detect_pull_hi = true
        },
        {   // sd_cards[1]
            .type = SD_IF_SDIO,
            .sdio_if_p = &sdio_if,
            // SD Card detect:
            .use_card_detect = true,
            .card_detect_gpio = 22,
            .card_detected_true = 0,  // What the GPIO read returns when a card is present.
            .card_detect_use_pull = true,
            .card_detect_pull_hi = true
        }
    };

    FatFsNs::FatFs::add_sd_card(&sd_cards[0]);
    FatFsNs::FatFs::add_sd_card(&sd_cards[1]);

    // The H/W config must be set up before this is called:
    sd_init_driver(); 
}

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define CHK_FRESULT(s, fr)                                       \
    if (FR_OK != fr) {                                           \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

void local_ls(const char *dir) {
    char cwdbuf[FF_LFN_BUF] = {0};
    FRESULT fr; /* Return value */
    char const *dir_str;
    if (dir[0]) {
        dir_str = dir;
    } else {
        fr = FatFsNs::Dir::getcwd(cwdbuf, sizeof cwdbuf);
        CHK_FRESULT("getcwd", fr);
        dir_str = cwdbuf;
    }
    cout << "Directory Listing: " << dir_str << endl;
    FILINFO fno = {}; /* File information */
    FatFsNs::Dir dirobj;
    fr = dirobj.findfirst(&fno, dir_str, "*");
    CHK_FRESULT("findfirst", fr);
    while (fr == FR_OK && fno.fname[0]) { /* Repeat while an item is found */
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        const char *pcWritableFile = "writable file",
                   *pcReadOnlyFile = "read only file",
                   *pcDirectory = "directory";
        const char *pcAttrib;
        /* Point pcAttrib to a string that describes the file. */
        if (fno.fattrib & AM_DIR) {
            pcAttrib = pcDirectory;
        } else if (fno.fattrib & AM_RDO) {
            pcAttrib = pcReadOnlyFile;
        } else {
            pcAttrib = pcWritableFile;
        }
        /* Create a string that includes the file name, the file size and the
         attributes string. */
        cout << fno.fname << " [" << pcAttrib << "]"
             << "[size=" << fno.fsize << "]" << endl;

        fr = dirobj.findnext(&fno); /* Search for next item */
    }
    dirobj.closedir();
}

static void test(FatFsNs::SdCard *SdCard_p) {
    FRESULT fr;

    cout << endl << "Testing drive " << SdCard_p->get_name() << endl;

    fr = SdCard_p->mount();
    CHK_FRESULT("mount", fr);
    fr = FatFsNs::FatFs::chdrive(SdCard_p->get_name());
    CHK_FRESULT("chdrive", fr);
    local_ls(NULL);

    FatFsNs::File file;
    fr = file.open("filename.txt", FA_OPEN_APPEND | FA_WRITE);
    CHK_FRESULT("open", fr);
    {
        char const *const str = "Hello, world!\n";
        if (file.printf(str) < strlen(str)) {
            cout << "printf failed" << endl;
            ;
            for (;;) __breakpoint();
        }
    }
    fr = file.close();
    CHK_FRESULT("close", fr);

    local_ls("/");

    fr = FatFsNs::Dir::mkdir("subdir");
    if (FR_OK != fr && FR_EXIST != fr) {
        cout << "mkdir error: " << FRESULT_str(fr) << "(" << fr << ")" << endl;
        for (;;) __breakpoint();
    }
    fr = FatFsNs::Dir::chdir("subdir");
    CHK_FRESULT("chdir", fr);
    fr = file.open("filename2.txt", FA_OPEN_APPEND | FA_WRITE);
    CHK_FRESULT("open", fr);
    {
        char const *const str = "Hello again\n";
        UINT bw;
        fr = file.write(str, strlen(str) + 1, &bw);
        CHK_FRESULT("write", fr);
        if (strlen(str) + 1 != bw) {
            cout << "Short write!" << endl;
            ;
            for (;;) __breakpoint();
        }
    }
    fr = file.close();
    CHK_FRESULT("close", fr);

    local_ls(NULL);

    fr = FatFsNs::Dir::chdir("/");
    CHK_FRESULT("chdir", fr);

    local_ls(NULL);

    fr = SdCard_p->unmount();
    CHK_FRESULT("unmount", fr);
}

void loop() {
    for (size_t i = 0; i < FatFsNs::FatFs::SdCard_get_num(); ++i)
        test(FatFsNs::FatFs::SdCard_get_by_num(i));
    sleep_ms(1000);
}