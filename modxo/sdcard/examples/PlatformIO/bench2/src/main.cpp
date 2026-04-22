/* Ported from: https://github.com/greiman/SdFat/blob/master/examples/bench/bench.ino
 *
 * This program is a simple binary write/read benchmark.
 *
 * Warning: this might destroy any data on the SD card(s), depending on configuration.
 */

#include <stdio.h>

#include "Arduino.h"
#include "FatFsSd.h"
#include "my_debug.h"

using namespace FatFsNs;

/* Implement library message callbacks */
void put_out_error_message(const char *s) {
    Serial1.printf("%s\r", s);
}
void put_out_info_message(const char *s) {
    Serial1.printf("%s\r", s);
}
/* This will not be called unless build_flags include "-D USE_DBG_PRINTF": */
// void put_out_debug_message(const char *s) {
//     Serial1.printf("%s\r", s);
// }
#define printf Serial1.printf
#define puts Serial1.println

#define error(s)                  \
    {                             \
        printf("ERROR: %s\r\n", s); \
        for (;;) __breakpoint();  \
    }

/* ********************************************************************** */

// Whether or not to format the card(s) in setup():
static const bool FORMAT = false;

// Set PRE_ALLOCATE true to pre-allocate file clusters.
static const bool PRE_ALLOCATE = true;

// Set SKIP_FIRST_LATENCY true if the first read/write to the SD can
// be avoid by writing a file header or reading the first record.
static const bool SKIP_FIRST_LATENCY = true;

// Size of read/write.
// static const size_t BUF_SIZE = 512;
#define BUF_SIZE (20 * 1024)

// File size in MiB where MiB = 1048576 bytes.
static const uint32_t FILE_SIZE_MiB = 5;

// Write pass count.
static const uint8_t WRITE_COUNT = 2;

// Read pass count.
static const uint8_t READ_COUNT = 2;
//==============================================================================
// End of configuration constants.
//------------------------------------------------------------------------------
// File size in bytes.
// static const uint32_t FILE_SIZE = 1000000UL * FILE_SIZE_MB;
static const uint32_t FILE_SIZE = (1024 * 1024 * FILE_SIZE_MiB);

// First (if s is not NULL and *s is not a null byte ('\0')) the argument string s is printed,
//   followed by a colon and a blank. Then the FRESULT error message and a new-line.
static void chk_result(const char* s, FRESULT fr) {
    if (FR_OK != fr) {
        if (s && *s)
            printf("%s: %s (%d)\r\n", s, FRESULT_str(fr), fr);
        else
            printf("%s (%d)\r\n", FRESULT_str(fr), fr);
        for (;;) __breakpoint();
    }
}

void setup() {
    // put your setup code here, to run once:

    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;                     // Serial is via USB; wait for enumeration
    printf("\033[2J\033[H");  // Clear Screen
    printf("\nUse a freshly formatted SD for best performance.\r\n");

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

    FatFs::add_sd_card(&sd_cards[0]);
    FatFs::add_sd_card(&sd_cards[1]);

    if (!FatFs::begin())
        error("Driver initialization failed\r\n");

    if (FORMAT) {
        for (size_t i = 0; i < FatFs::SdCard_get_num(); ++i) {
            SdCard* SdCard_p = FatFs::SdCard_get_by_num(i);
            printf("Formatting drive %s...\r\n", SdCard_p->get_name());
            FRESULT fr = SdCard_p->format();
            chk_result("format", fr);
        }
    }
}

//------------------------------------------------------------------------------
/**
 * Benchmarks the SD card by writing and reading a file.
 *
 * @param logdrv The logical drive name of the SD card to benchmark.
 */
static void bench(char const* logdrv) {
    File file;
    float s;
    uint32_t t;
    uint32_t maxLatency;
    uint32_t minLatency;
    uint32_t totalLatency;
    bool skipLatency;

    static_assert(0 == FILE_SIZE % BUF_SIZE,
                  "For accurate results, FILE_SIZE must be a multiple of BUF_SIZE.");

    // Insure 4-byte alignment.
    uint32_t buf32[BUF_SIZE] __attribute__((aligned(4)));
    uint8_t* buf = (uint8_t*)buf32;

    SdCard* SdCard_p(FatFs::SdCard_get_by_name(logdrv));
    if (!SdCard_p) {
        printf("Unknown logical drive name: %s\r\n", logdrv);
        for (;;) __breakpoint();
    }
    FRESULT fr = f_chdrive(logdrv);
    chk_result("f_chdrive", fr);

    switch (SdCard_p->fatfs()->fs_type) {
        case FS_EXFAT:
            printf("Type is exFAT\r\n");
            break;
        case FS_FAT12:
            printf("Type is FAT12\r\n");
            break;
        case FS_FAT16:
            printf("Type is FAT16\r\n");
            break;
        case FS_FAT32:
            printf("Type is FAT32\r\n");
            break;
    }

    printf("Card size: ");
    printf("%.2f", SdCard_p->get_num_sectors() * 512E-9);
    printf(" GB (GB = 1E9 bytes)\r\n");

    // typedef int (*printer_t)(const char* format, ...);

    SdCard_p->cidDmp(info_message_printf);
    SdCard_p->csdDmp(info_message_printf);

    SdCard_p->mount();
    chk_result("f_mount", fr);

    // fill buf with known data
    if (BUF_SIZE > 1) {
        for (size_t i = 0; i < (BUF_SIZE - 2); i++) {
            buf[i] = 'A' + (i % 26);
        }
        buf[BUF_SIZE - 2] = '\r';
    }
    buf[BUF_SIZE - 1] = '\n';

    // Open or create file.
    // FA_CREATE_ALWAYS:
    //	Creates a new file.
    //  If the file is existing, it will be truncated and overwritten.
    fr = file.open("bench.dat", FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    chk_result("open", fr);

    // Open with FA_CREATE_ALWAYS creates a new file,
    // and if the file is existing, it will be truncated and overwritten.
    if (PRE_ALLOCATE) {
        fr = file.expand(FILE_SIZE);
        chk_result("file.expand", fr);
    }
    printf("FILE_SIZE_MB = %lu\r\n", FILE_SIZE_MiB);
    printf("BUF_SIZE = %zu\r\n", BUF_SIZE);
    printf("Starting write test, please wait.\n\r\n");

    // do write test
    uint32_t n = FILE_SIZE / BUF_SIZE;
    printf("write speed and latency\r\n");
    printf("speed,max,min,avg\r\n");
    printf("KB/Sec,usec,usec,usec\r\n");
    for (uint8_t nTest = 0; nTest < WRITE_COUNT; nTest++) {
        fr = file.rewind();
        chk_result("file.rewind", fr);

        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            uint32_t m = micros();

            unsigned int bw;
            fr = file.write(buf, BUF_SIZE, &bw); /* Write it to the destination file */
            chk_result("file.write", fr);
            if (bw < BUF_SIZE) { /* error or disk full */
                error("write failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (skipLatency) {
                // Wait until first write to SD, not just a copy to the cache.
                skipLatency = file.tell() < 512;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        fr = file.sync();
        chk_result("file.sync", fr);

        t = millis() - t;
        s = file.size();
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        printf(",%lu\r\n", totalLatency / n);
    }
    printf("\nStarting read test, please wait.\r\n");
    printf("\nread speed and latency\r\n");
    printf("speed,max,min,avg\r\n");
    printf("KB/Sec,usec,usec,usec\r\n");

    // do read test
    for (uint8_t nTest = 0; nTest < READ_COUNT; nTest++) {
        fr = file.rewind();
        chk_result("file.rewind", fr);
        maxLatency = 0;
        minLatency = 9999999;
        totalLatency = 0;
        skipLatency = SKIP_FIRST_LATENCY;
        t = millis();
        for (uint32_t i = 0; i < n; i++) {
            buf[BUF_SIZE - 1] = 0;
            uint32_t m = micros();
            unsigned int nr;
            fr = file.read(buf, BUF_SIZE, &nr);
            chk_result("file.read", fr);
            if (nr != BUF_SIZE) {
                error("read failed");
            }
            m = micros() - m;
            totalLatency += m;
            if (buf[BUF_SIZE - 1] != '\n') {
                error("data check error");
            }
            if (skipLatency) {
                skipLatency = false;
            } else {
                if (maxLatency < m) {
                    maxLatency = m;
                }
                if (minLatency > m) {
                    minLatency = m;
                }
            }
        }
        s = file.size();
        t = millis() - t;
        printf("%.1f,%lu,%lu", s / t, maxLatency, minLatency);
        printf(",%lu\r\n", totalLatency / n);
    }
    printf("\nDone\r\n");
    fr = file.close();
    chk_result("file.close", fr);
    fr = SdCard_p->unmount();
    chk_result("file.unmount", fr);
}

void loop() {
    // put your main code here, to run repeatedly:
    printf("\nTesting drive 0:\r\n");
    bench("0:");
    printf("\nTesting drive 1:\r\n");
    bench("1:");
}
