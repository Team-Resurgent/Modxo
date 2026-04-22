/* big_file_test.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "pico/stdlib.h"
#include "pico/types.h"
//
#include "f_util.h"
#include "my_debug.h"

#define FF_MAX_SS 512
#define BUFFSZ (64 * FF_MAX_SS)  // Should be a factor of 1 Mebibyte

#define PRE_ALLOCATE true

typedef uint32_t DWORD;
typedef unsigned int UINT;

static void report(uint64_t size, uint64_t elapsed_us) {
    double elapsed = (double)elapsed_us / 1000 / 1000;
    IMSG_PRINTF("Elapsed seconds %.3g\n", elapsed);
    IMSG_PRINTF("Transfer rate ");
    if ((double)size / elapsed / 1024 / 1024 > 1.0) {
        IMSG_PRINTF("%.3g MiB/s (%.3g MB/s), or ",
               (double)size / elapsed / 1024 / 1024,
               (double)size / elapsed / 1000 / 1000);
    }
    IMSG_PRINTF("%.3g KiB/s (%.3g kB/s) (%.3g kb/s)\n",
           (double)size / elapsed / 1024, (double)size / elapsed / 1000, 8.0 * size / elapsed / 1000);
}

// Create a file of size "size" bytes filled with random data seeded with "seed"
static bool create_big_file(const char *const pathname, uint64_t size,
                            unsigned seed, DWORD *buff) {
    FRESULT fr;
    FIL file; /* File object */

    srand(seed);  // Seed pseudo-random number generator

    /* Open the file, creating the file if it does not already exist. */
    fr = f_open(&file, pathname, FA_WRITE | FA_CREATE_ALWAYS);
    if (FR_OK != fr) {
        EMSG_PRINTF("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        f_close(&file);
        return false;
    }
    if (PRE_ALLOCATE) {
#if 0
        FRESULT fr = f_lseek(&file, size);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_lseek error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
        if (f_tell(&file) != size) {
            EMSG_PRINTF("Disk full?\n");
            f_close(&file);
            return false;
        }
        fr = f_rewind(&file);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_rewind error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
#endif
        fr = f_truncate(&file);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_truncate error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
        // prepares or allocates a contiguous data area to the file:
        fr = f_expand(&file, size, 1);
        if (FR_OK != fr) {
            EMSG_PRINTF("f_expand error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
    }

    IMSG_PRINTF("Writing...\n");

    uint64_t cum_time = 0;

    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        size_t n;
        for (n = 0; n < BUFFSZ / sizeof(DWORD); n++) buff[n] = rand();
        UINT bw;

        absolute_time_t xStart = get_absolute_time();
        fr = f_write(&file, buff, BUFFSZ, &bw);
        if (bw < BUFFSZ) {
            EMSG_PRINTF("f_write(%s,,%d,): only wrote %d bytes\n", pathname, BUFFSZ, bw);
            f_close(&file);
            return false;
        }
        if (FR_OK != fr) {
            EMSG_PRINTF("f_write error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
        cum_time += absolute_time_diff_us(xStart, get_absolute_time());
    }
    /* Close the file */
    f_close(&file);

    report(size, cum_time);
    return true;
}

// Read a file of size "size" bytes filled with random data seeded with "seed"
// and verify the data
static bool check_big_file(char *pathname, uint64_t size,
                           uint32_t seed, DWORD *buff) {
    FRESULT fr;
    FIL file; /* File object */

    srand(seed);  // Seed pseudo-random number generator

    fr = f_open(&file, pathname, FA_READ);
    if (FR_OK != fr) {
        EMSG_PRINTF("f_open error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    IMSG_PRINTF("Reading...\n");

    uint64_t cum_time = 0;

    for (uint64_t i = 0; i < size / BUFFSZ; ++i) {
        UINT br;

        absolute_time_t xStart = get_absolute_time();
        fr = f_read(&file, buff, BUFFSZ, &br);
        if (br < BUFFSZ) {
            EMSG_PRINTF("f_read(,%s,%d,):only read %u bytes\n", pathname, BUFFSZ, br);
            f_close(&file);
            return false;
        }
        if (FR_OK != fr) {
            EMSG_PRINTF("f_read error: %s (%d)\n", FRESULT_str(fr), fr);
            f_close(&file);
            return false;
        }
        cum_time += absolute_time_diff_us(xStart, get_absolute_time());

        /* Check the buffer is filled with the expected data. */
        size_t n;
        for (n = 0; n < BUFFSZ / sizeof(DWORD); n++) {
            unsigned int expected = rand();
            unsigned int val = buff[n];
            if (val != expected) {
                EMSG_PRINTF("Data mismatch at dword %llu: expected=0x%8x val=0x%8x\n",
                       (i * sizeof(buff)) + n, expected, val);
                f_close(&file);
                return false;
            }
        }
    }
    /* Close the file */
    f_close(&file);

    report(size, cum_time);
    return true;
}
// Specify size in Mebibytes (1024x1024 bytes)
void big_file_test(char *pathname, size_t size_MiB, uint32_t seed) {
    //  /* Working buffer */
    DWORD *buff = malloc(BUFFSZ);
    myASSERT(buff);
    if (!buff) abort();
    myASSERT(size_MiB);
    if (4095 < size_MiB) {
        EMSG_PRINTF("Warning: Maximum file size: 2^32 - 1 bytes on FAT volume\n");
    }
    uint64_t size_B = (uint64_t)size_MiB * 1024 * 1024;

    if (create_big_file(pathname, size_B, seed, buff))
        check_big_file(pathname, size_B, seed, buff);

    free(buff);
}

/* [] END OF FILE */
