// lib/tinyusb/examples/device/cdc_msc/src/msc_disk.c
/*
 * Modifications copyright 2024 Carl John Kugler III
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may not use
 * this file except in compliance with the License. You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <tusb.h>
//
#include <pico/stdlib.h>
//
#include "diskio.h" /* Declarations of disk functions */
#include "my_debug.h"

static bool ejected = false;  // FIXME: should be LUN specific

// #define TRACE_PRINTF printf
#define TRACE_PRINTF(fmt, args...)

/**
 * @brief Handles the INQUIRY request.
 *
 * @param lun The LUN that the request is for.
 * @param vendor_id The vendor ID to return.
 * @param product_id The product ID to return.
 * @param product_rev The product revision to return.
 */
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16],
                        uint8_t product_rev[4]) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    (void)lun;
    // FIXME: maybe this should come from the SD card's CSD?
    const char vid[] = "Pico SD";
    const char pid[] = "Mass Storage";
    const char rev[] = "1.0";

    // Copy the vendor ID, product ID, and revision to the buffers.
    snprintf((char*)vendor_id, 8, "%s", vid);
    snprintf((char*)product_id, 16, "%s", pid);
    snprintf((char*)product_rev, 4, "%s", rev);
}

/**
 * @brief Checks if the unit is ready.
 *
 * @param[in] lun The logical unit number that the request is for.
 *
 * @retval true The unit is ready.
 * @retval false The unit is not ready.
 */
bool tud_msc_test_unit_ready_cb(uint8_t lun) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    DSTATUS ds = disk_initialize(lun);
    return (!(STA_NOINIT & ds) && !(STA_NODISK & ds));
}

/**
 * @brief Gets the capacity of the disk.
 *
 * @param[in] lun The logical unit number that the request is for.
 * @param[out] block_count_p The number of blocks in the disk.
 * @param[out] block_size_p The size of each block in bytes.
 *
 * @return true if the request was successful, false otherwise.
 */
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count_p, uint16_t* block_size_p) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    if (!tud_msc_test_unit_ready_cb(lun)) {
        *block_count_p = 0;
    } else {
        DRESULT dr = disk_ioctl(lun, GET_SECTOR_COUNT, block_count_p);
        if (RES_OK != dr) *block_count_p = 0;
    }
    *block_size_p = 512;
}

/**
 * @brief Callback for the Mass Storage Class's START STOP UNIT request.
 *
 * This callback is invoked when the host sends a START STOP UNIT request to the device.
 *
 * @param[in] lun The logical unit number that the request is for.
 * @param[in] power_condition The power condition of the request.
 * @param[in] start true if the request is to start the unit, false if it is to stop it.
 * @param[in] load_eject true if the request is to load or eject the media.
 *
 * @retval true if the request was successful, false otherwise.
 */
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    (void)power_condition;
    if (load_eject) {
        if (start) {
            // load disk storage
            ejected = false;
        } else {
            // unload disk storage
            DRESULT dr = disk_ioctl(lun, CTRL_SYNC, 0);
            if (RES_OK != dr) return false;
            ejected = true;
        }
    }
    return true;
}
/**
 * @brief Reads data from the disk.
 *
 * This callback is invoked when the host sends a READ (10) request to the device.
 *
 * @param[in] lun The logical unit number that the request is for.
 * @param[in] lba The logical block address to read from.
 * @param[in] offset The offset in bytes from the beginning of the block to read from.
 * @param[out] buffer The buffer to store the read data in.
 * @param[in] bufsize The size of the buffer in bytes.
 *
 * @return The number of bytes read from the disk.
 */
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer,
                          uint32_t bufsize) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    (void)offset;
    assert(!offset);
    assert(!(bufsize % 512));

    if (ejected) return -1;
    if (!tud_msc_test_unit_ready_cb(lun)) return -1;

    // Read data from the disk.
    DRESULT dr = disk_read(lun, (BYTE*)buffer, lba, bufsize / 512);
    if (RES_OK != dr) return -1;

    return (int32_t)bufsize;
}

/**
 * @brief Returns true if the MSC is writable.
 *
 * @param[in] lun The logical unit number that the request is for.
 *
 * @return true if the MSC is writable, false otherwise.
 */
bool tud_msc_is_writable_cb(uint8_t lun) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    if (ejected) return false;

    DSTATUS ds = disk_status(lun);
    return !(STA_PROTECT & ds);
}

/**
 * @brief Write data to the disk.
 *
 * @param[in] lun The logical unit number that the request is for.
 * @param[in] lba The logical block address to write to.
 * @param[in] offset The offset in bytes from the beginning of the
 *                    block to write to.
 * @param[in] buffer The buffer containing the data to be written.
 * @param[in] bufsize The size of the buffer in bytes.
 *
 * @return The number of bytes written.
 */
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer,
                           uint32_t bufsize) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    (void)offset;
    assert(!offset);
    assert(!(bufsize % 512));

    if (ejected) return -1;
    if (!tud_msc_test_unit_ready_cb(lun)) return -1;

    // Write data to the disk.
    DRESULT dr = disk_write(lun, (BYTE*)buffer, lba, bufsize / 512);
    if (RES_OK != dr) return -1;

    return bufsize;
}

/**
 * @brief Callback for handling SCSI commands.
 *
 * @param[in] lun The logical unit number that the request is for.
 * @param[in] scsi_cmd The SCSI command to be handled.
 * @param[in] buffer The buffer containing the data to be written.
 * @param[in] bufsize The size of the buffer in bytes.
 *
 * @return The number of bytes written.
 */
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer,
                        uint16_t bufsize) {
    TRACE_PRINTF("%s(lun=%d)\n", __func__, lun);
    void const* response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    switch (scsi_cmd[0]) {
        default:
            // Set Sense = Invalid Command Operation
            tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
            // negative means error -> tinyusb could stall and/or response with failed status
            resplen = -1;
            break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) resplen = bufsize;

    if (response && (resplen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, (size_t)resplen);
        } else {
            ;  // SCSI output
        }
    }
    return (int32_t)resplen;
}
