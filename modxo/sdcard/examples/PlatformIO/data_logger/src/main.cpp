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

/*
This example reads analog input A0 and logs the voltage
  in a file on SD cards once per second.

It also demonstates a way to do static configuration.
*/
#include <stdlib.h>
#include <time.h>
//
#include "hardware/adc.h"
#include "pico/aon_timer.h"
#include "pico/stdlib.h"
//
#include "FatFsSd.h"
#include "SerialUART.h"
#include "iostream/ArduinoStream.h"

// Serial output stream
ArduinoOutStream cout(Serial1);

using namespace FatFsNs;

/* Implement library message callbacks */
void put_out_error_message(const char *s) {
    Serial1.write(s);
}
void put_out_info_message(const char *s) {
    Serial1.write(s);
}
/* This will not be called unless build_flags include "-D USE_DBG_PRINTF": */
// void put_out_debug_message(const char *s) {
//     Serial1.write(s);
// }

/* ********************************************************************** */

// Check the FRESULT of a library call.
//  (See http://elm-chan.org/fsw/ff/doc/rc.html.)
#define FAIL(s, fr)                                              \
    {                                                            \
        cout << __FILE__ << ":" << __LINE__ << ": " << s << ": " \
             << FRESULT_str(fr) << " (" << fr << ")" << endl;    \
        for (;;) __breakpoint();                                 \
    }

static void CHK_FRESULT(const char* s, FRESULT fr) {
    if (FR_OK != fr)
        FAIL(s, fr);
}

#define ASSERT(pred)                                       \
    {                                                      \
        if (!(pred)) {                                     \
            cout << __FILE__ << ":" << __LINE__ << ": "    \
                 << "Assertion failed: " << #pred << endl; \
            for (;;) __breakpoint();                       \
        }                                                  \
    }

/* ********************************************************************** */

static bool print_heading(File& file) {
    FRESULT fr = file.lseek(file.size());
    CHK_FRESULT("lseek", fr);
    if (0 == file.tell()) {
        // Print header
        if (file.printf("Date,Time,Voltage\n") < 0) {
            cout << "printf error" << endl;
            return false;
        }
    }
    return true;
}

static bool open_file(File& file) {
    const time_t timer = time(NULL);
    struct tm tmbuf;
    localtime_r(&timer, &tmbuf);
    char filename[64];
    int n = snprintf(filename, sizeof filename, "/data");
    ASSERT(0 < n && n < (int)sizeof filename);
    FRESULT fr = Dir::mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("mkdir", fr);
        return false;
    }
    //  tm_year	int	years since 1900
    //  tm_mon	int	months since January	0-11
    //  tm_mday	int	day of the month	1-31
    n += snprintf(filename + n, sizeof filename - n, "/%04d-%02d-%02d",
                  tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday);
    ASSERT(0 < n && n < (int)sizeof filename);
    fr = Dir::mkdir(filename);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("mkdir", fr);
        return false;
    }
    size_t nw = strftime(filename + n, sizeof filename - n, "/%H.csv", &tmbuf);
    ASSERT(nw);
    fr = file.open(filename, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        FAIL("open", fr);
        return false;
    }
    if (!print_heading(file)) return false;
    return true;
}

bool process_logger() {
    /* It's very inefficient to open and close the file for every record,
    but you're less likely to lose data that way.  But also see f_sync
    (http://elm-chan.org/fsw/ff/doc/sync.html). */

    FRESULT fr;
    File file;

    bool rc = open_file(file);
    if (!rc) return false;

    // Form date-time string
    char buf[128];
    const time_t secs = time(NULL);
    struct tm tmbuf;
    struct tm* ptm = localtime_r(&secs, &tmbuf);
    size_t n = strftime(buf, sizeof buf, "%F,%T,", ptm);
    ASSERT(n);

    /* Assuming something analog is connected to A0 */
    int sensorValue = analogRead(A0);
    float voltage = 3.3f * sensorValue / 1024;
    int nw = snprintf(buf + n, sizeof buf - n, "%.3f\n", (double)voltage);
    // Notice that only when this returned value is non-negative and less than n,
    //   the string has been completely written.
    ASSERT(0 < nw && nw < (int)sizeof buf);
    n += nw;
    cout << buf << "\r";

    UINT bw;
    fr = file.write(buf, n, &bw);
    CHK_FRESULT("write", fr);
    if (bw < n) {
        cout << "Short write!" << endl;
        for (;;) __breakpoint();
    }
    fr = file.close();
    CHK_FRESULT("close", fr);
    return true;
}

static bool logger_enabled;
static const uint32_t period = 1000;
static absolute_time_t next_log_time;

void setup() {
    Serial1.begin(115200);  // set up Serial library at 9600 bps
    while (!Serial1)
        ;  // Serial is via USB; wait for enumeration
    cout << "Hello, world!" << endl;

    adc_init(); // Reading voltage on A0

    time_init();
    // You might want to ask the user for the time,
    //   but it is hardcoded here for simplicity:
    struct tm t = {
        // tm_sec	int	seconds after the minute	0-61*
        .tm_sec = 0,
        // tm_min	int	minutes after the hour	0-59
        .tm_min = 31,
        // tm_hour	int	hours since midnight	0-23
        .tm_hour = 16,
        // tm_mday	int	day of the month	1-31
        .tm_mday = 22,
        // tm_mon	int	months since January	0-11
        .tm_mon = 9 - 1,
        // tm_year	int	years since 1900
        .tm_year = 2024 - 1900,
        // tm_wday	int	days since Sunday	0-6
        .tm_wday = 0,
        // tm_yday	int	days since January 1	0-365
        .tm_yday = 0,
        // tm_isdst	int	Daylight Saving Time flag
        .tm_isdst = 0
    };
    /* The values of the members tm_wday and tm_yday of timeptr are ignored, and the values of
       the other members are interpreted even if out of their valid ranges */
    time_t epoch_secs = mktime(&t);
    if (-1 == epoch_secs) {
        printf("The passed in datetime was invalid\n");
        abort();
    }
    struct timespec ts = {.tv_sec = epoch_secs, .tv_nsec = 0};
    aon_timer_set_time(&ts);

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
        .spi = &spi,    // Pointer to the SPI driving this card
        .ss_gpio = 12,  // The SPI slave select GPIO for this SD card
        .set_drive_strength = true,
        .ss_gpio_drive_strength = GPIO_DRIVE_STRENGTH_4MA};

    // Hardware Configuration of the SD Card object:
    static sd_card_t sd_card = {
        .type = SD_IF_SPI,
        .spi_if_p = &spi_if,  // Pointer to the SPI interface driving this card
        // SD Card detect:
        .use_card_detect = true,
        .card_detect_gpio = 14,
        .card_detected_true = 0,  // What the GPIO read returns when a card is present.
        .card_detect_use_pull = true,
        .card_detect_pull_hi = true};

    FatFsNs::SdCard* SdCard_p(FatFsNs::FatFs::add_sd_card(&sd_card));

    // The H/W config must be set up before this is called:
    sd_init_driver(); 

    FRESULT fr = SdCard_p->mount();
    CHK_FRESULT("mount", fr);

    next_log_time = delayed_by_ms(get_absolute_time(), period);
    logger_enabled = true;
}

void loop() {
    if (absolute_time_diff_us(get_absolute_time(), next_log_time) < 0) {
        FRESULT fr;
        if (logger_enabled) {
            if (!process_logger())
                logger_enabled = false;
        }
        next_log_time = delayed_by_ms(next_log_time, period);
    }
}
