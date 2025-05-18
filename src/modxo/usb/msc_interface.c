/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, QuantX <Sam Deutsch>
*/
#include "msc_interface.h"
#include "tusb.h"

typedef struct
{
    uint32_t block_count;
    uint32_t block_size;
    const char * product_str;
    msc_handler_cback read_cback;
    msc_handler_cback write_cback;
} msc_handler;

#define MSC_HANDLER_MAX_ENTRIES 16 // This is the maximum number of allowed LUNs
static msc_handler msc_hdlr_table[MSC_HANDLER_MAX_ENTRIES];
static uint8_t msc_hdlr_count = 0;

// Invoked to determine max LUN
uint8_t tud_msc_get_maxlun_cb(void)
{
    return msc_hdlr_count;
}

const char * vid = "ModXo";
const char * pid = "Mass Storage";
const char * rev = "1.0";

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
    const char * product_str = pid;
    if (lun < msc_hdlr_count && msc_hdlr_table[lun].product_str)
    {
        product_str = msc_hdlr_table[lun].product_str;
    }

    int product_len = strlen(product_str);
    if (product_len > 16) product_len = 16;

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, product_str, product_len);
    memcpy(product_rev, rev, strlen(rev));
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
    // if ( lun == 1 && board_button_read() ) return false;

    return true; // RAM disk is always ready
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
    if (lun >= msc_hdlr_count) {
        *block_count = 0;
        *block_size = 0;
        return;
    }

    *block_count = msc_hdlr_table[lun].block_count;
    *block_size = msc_hdlr_table[lun].block_size;
}

// Invoked when received Start Stop Unit command
// - Start = 0 : stopped power mode, if load_eject = 1 : unload disk storage
// - Start = 1 : active mode, if load_eject = 1 : load disk storage
bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
    (void) lun;
    (void) power_condition;

    return true;
}

bool tud_msc_is_writable_cb(uint8_t lun)
{
    if (lun >= msc_hdlr_count) return false;

    // If no write callback is registered then this LUN is not writeable
    return msc_hdlr_table[lun].write_cback != NULL;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and return number of copied bytes.
int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
    if (lun >= msc_hdlr_count) return -1;

    if (msc_hdlr_table[lun].read_cback)
        return msc_hdlr_table[lun].read_cback(lba, offset, buffer, bufsize);

    return -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and return number of written bytes
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
    if (lun >= msc_hdlr_count) return -1;

    if (msc_hdlr_table[lun].write_cback)
        return msc_hdlr_table[lun].write_cback(lba, offset, buffer, bufsize);
    
    return -1;
}

// Callback invoked when received an SCSI command not in built-in list below
// - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, MODE_SENSE6, REQUEST_SENSE
// - READ10 and WRITE10 has their own callbacks (MUST not be handled here)
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{
    (void) buffer;
    (void) bufsize;

    switch (scsi_cmd[0]) {
    default:
        // Set Sense = Invalid Command Operation
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
    }

    // negative means error -> tinyusb could stall and/or response with failed status
    return -1;
}

bool msc_interface_add_handler(uint32_t block_count, uint32_t block_size, const char * product_str, msc_handler_cback read_cback, msc_handler_cback write_cback)
{
    if (msc_hdlr_count >= MSC_HANDLER_MAX_ENTRIES) return false;

    if (!read_cback) return false; // A read callback must be present, write callback is optional

    msc_hdlr_table[msc_hdlr_count].block_count = block_count;
    msc_hdlr_table[msc_hdlr_count].block_size = block_size;
    msc_hdlr_table[msc_hdlr_count].product_str = product_str;
    msc_hdlr_table[msc_hdlr_count].read_cback = read_cback;
    msc_hdlr_table[msc_hdlr_count].write_cback = write_cback;

    msc_hdlr_count++;

    return true;
}