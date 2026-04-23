//
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
//
#include "pico/stdlib.h"
//
#include "ff.h"
//
#include "delays.h"
#include "f_util.h"
#include "file_stream.h"
#include "hw_config.h"

#define DRIVE "0:"
#define ITERATIONS 100000

#if defined(NDEBUG)
#  pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static inline void stop() {
    fflush(NULL);
    exit(-1);
}

// See FatFs - Generic FAT Filesystem Module, "Application Interface",
// http://elm-chan.org/fsw/ff/00index_e.html
static void unbuffered() {
    uint64_t start = micros();
    uint64_t then = start;

    FIL fil = {};
    FRESULT fr = f_open(&fil, DRIVE "/times_ub.csv", FA_CREATE_ALWAYS | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr)
        panic("f_open error: %s\n", FRESULT_str(fr));
    for (size_t i = 0; i < ITERATIONS; ++i) {
        uint64_t now = micros();

        if (f_printf(&fil, "%llu, %llu\n", now - start, now - then) < 0) {
            printf("f_printf failed\n");
            stop();
        }
        then = now;
    }    
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s\n", FRESULT_str(fr));
    }
    printf("Time without buffering: %.3g ms\n", (float)(micros() - start) / 1000);
}

// See https://sourceware.org/newlib/libc.html
static void buffered() {
    uint64_t start = micros();
    uint64_t then = start;

    FILE *file_p = open_file_stream(DRIVE "/times_b.csv", "w");
    if (!file_p) {
        perror("fopen");
        stop();
    }
    /* setvbuf() is optional, but alignment is critical for good
    performance with SDIO because it uses DMA with word width.
    Also, the buffer size should be a multiple of the SD block 
    size, 512 bytes. */
    static char vbuf[1024] __attribute__((aligned));
    int err = setvbuf(file_p, vbuf, _IOFBF, sizeof vbuf);
    assert(!err);

    for (size_t i = 0; i < ITERATIONS; ++i) {
        uint64_t now = micros();
        if (fprintf(file_p, "%llu, %llu\n", now - start, now - then) < 0) {
            perror("fprintf");
            stop();
        }
        then = now;
    }    
    if (-1 == fclose(file_p)) {
        perror("fclose");
        stop();
    }
    printf("Time with buffering: %.3g ms\n", (float)(micros() - start) / 1000);
}

int main() {
    stdio_init_all();

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html
    FATFS fs = {};
    FRESULT fr = f_mount(&fs, DRIVE, 1);
    if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);

    unbuffered();
    buffered();

    f_unmount(DRIVE);

    puts("Goodbye, world!");
    for (;;);
}
