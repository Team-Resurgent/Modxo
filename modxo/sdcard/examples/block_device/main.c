/**
 * \file main.c
 *
 * \brief Example program demonstrating how to use the low-level disk I/O
 * functions provided by the SD driver.
 *
 * This program writes two blocks of data, reads the blocks back, 
*  and verifies that the data matches what was written.
 *
 * \note This program will destroy any existing filesystem on the SD card.
 *
 * \author Carl John Kugler III
 *
 * \copyright Copyright 2024 Carl John Kugler III
 * \license Apache License, Version 2.0
 *
 * WARNING: Any filesystem format on the target drive will be destroyed!
 */

//
#include <assert.h>
#include <stdio.h>
#include <string.h>
//
#include "pico/stdlib.h"
//
#include "diskio.h" /* Declarations of disk functions */

#ifdef NDEBUG 
#   warning "This test relies on asserts to verify test results!"
#endif

int main() {
    stdio_init_all();

    puts("Hello, world!");

    // Physical drive number
    BYTE const pdrv = 0;

    // Initialize the disk subsystem
    DSTATUS ds = disk_initialize(pdrv);
    assert(0 == (ds & STA_NOINIT));

    // Get the number of sectors on the drive
    DWORD sz_drv = 0;
    DRESULT dr = disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_drv);
    assert(RES_OK == dr);

    // Create a type for a 512-byte block
    typedef char block_t[512];

    // Define a couple of blocks
    block_t blocks[2];

    // Initialize the blocks
    snprintf(blocks[0], sizeof blocks[0], "Hello");
    snprintf(blocks[1], sizeof blocks[1], "World!");

    LBA_t const lba = 0; // Arbitrary
    assert(lba + count_of(blocks) < sz_drv);

    // Write the blocks
    dr = disk_write(pdrv, (BYTE*)blocks[lba], lba, count_of(blocks));
    assert(RES_OK == dr);

    // Sync the disk
    dr = disk_ioctl(pdrv, CTRL_SYNC, 0);
    assert(RES_OK == dr);

    memset(blocks, 0xA5, sizeof blocks);

    // Read the blocks
    dr = disk_read(pdrv, (BYTE*)blocks[lba], lba, count_of(blocks));
    assert(RES_OK == dr);

    // Verify the data
    assert(0 == strcmp(blocks[0], "Hello"));
    assert(0 == strcmp(blocks[1], "World!"));

    puts("Goodbye, world!");

    for (;;) __breakpoint();
}
