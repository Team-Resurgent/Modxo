/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Team Resurgent, Shalx

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <modxo.h>
#include <modxo/modxo_queue.h>
#include <modxo/modxo_ports.h>
#include <modxo/lpc_interface.h>
#include <lpc_mem_window.h>

#include <pico.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>
#include <hardware/sync.h>

#include <modxo_pinout.h>

#if SD_CARD_SPI_ENABLE
#include "ff.h"
#endif

#define SDCARD_MAX_ENTRIES 8
#define SDCARD_QUEUE_BUFFER_LEN 8
#define SDCARD_PATH_MAX 280

#define SDCARD_FILE_CHUNK_SIZE_SHIFT 12
#define SDCARD_FILE_CHUNK_SIZE (1 << SDCARD_FILE_CHUNK_SIZE_SHIFT)

#define SDCARD_CWD_RESULT_OK 0
#define SDCARD_CWD_RESULT_BAD_INDEX 1
#define SDCARD_CWD_RESULT_NOT_DIRECTORY 2
#define SDCARD_CWD_RESULT_PATH_TOO_LONG 3
#define SDCARD_CWD_RESULT_AT_ROOT 4

#define SDCARD_FILE_RESULT_IDLE 0 
#define SDCARD_FILE_RESULT_OK 1
#define SDCARD_FILE_RESULT_ERROR 2
#define SDCARD_FILE_RESULT_INVALID_INDEX 3
#define SDCARD_FILE_RESULT_IS_DIRECTORY 4

#define SDCARD_COMMAND_NONE 0

#define SDCARD_COMMAND_REQUEST_REMOUNT 1
#define SDCARD_COMMAND_RESPONSE_REMOUNT_READY 2
#define SDCARD_COMMAND_RESPONSE_REMOUNT_RESULT 3

#define SDCARD_COMMAND_REQUEST_FILE_LIST 4 // set long value for offset
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_COUNT 5 
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_READY 6
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_RESULT 7

#define SDCARD_COMMAND_REQUEST_OPEN_FILE 8 // data = index of list
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_READY 9
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_RESULT 10

#define SDCARD_COMMAND_REQUEST_FLASH_SECTOR 11 // flashes loaded sector at current_long offset
#define SDCARD_COMMAND_RESPONSE_FLASH_SECTOR_READY 12
#define SDCARD_COMMAND_RESPONSE_FLASH_SECTOR_RESULT 13

#define SDCARD_COMMAND_CLOSE_FILE 14

#define SDCARD_COMMAND_FILE_READ_SECTOR 15 // set long value for sector index, sets long to length read

#define SDCARD_COMMAND_CWD_ROOT 16
#define SDCARD_COMMAND_CWD_PARENT 17
#define SDCARD_COMMAND_SET_CWD_FROM_INDEX 18
#define SDCARD_COMMAND_GET_FILE_NAME_FROM_INDEX 19
#define SDCARD_COMMAND_GET_FILE_SIZE_FROM_INDEX 20
#define SDCARD_COMMAND_GET_FILE_FLAGS_FROM_INDEX 21

#define SDCARD_COMMAND_SET_PAYLOAD_TYPE 22

#define SDCARD_PAYLOAD_TYPE_NONE 0
#define SDCARD_PAYLOAD_TYPE_FILE_NAME 1
#define SDCARD_PAYLOAD_TYPE_FILE_SD_SECTOR 2
#define SDCARD_PAYLOAD_TYPE_FILE_SD_BIOS 3

#pragma pack(push, 1)
typedef union
{
    struct
    {
        uint8_t cmd;
        uint8_t param1;
        uint8_t param2;
        uint8_t param3;
    };
    uint8_t bytes[4];
    uint32_t raw;
} MODXO_SDCARD_CMD;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    uint32_t id;
    uint8_t flags;
    uint32_t file_size;
    char name[SDCARD_PATH_MAX];
    uint16_t file_name_length;
} sdcard_file_entry_t;
#pragma pack(pop)

typedef struct
{
    MODXO_QUEUE_ITEM_T buffer[SDCARD_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;

    uint8_t remount_ready;
    uint8_t remount_result;

    uint32_t flash_sector_offset;
    uint8_t flash_sector_ready;
    uint8_t flash_sector_result;

    uint32_t file_list_offset;
    uint8_t file_list_count;
    uint8_t file_list_name_index;
    uint8_t file_list_ready;
    uint8_t file_list_result;
    sdcard_file_entry_t file_entries[SDCARD_MAX_ENTRIES];

    uint8_t open_file_index;
    uint32_t open_file_size;
    uint8_t open_file_ready;
    uint8_t open_file_result;
    FIL open_file;

    uint32_t cached_sector_index;
    uint32_t cached_sector_length;
    uint8_t cached_sector_buffer[SDCARD_FILE_CHUNK_SIZE];

    char cwd[SDCARD_PATH_MAX];
    uint8_t payload_type;

#if SD_CARD_SPI_ENABLE
    FATFS fatfs;
    uint8_t sd_fat_mounted;
#endif
} sdcard_state_t;

static sdcard_state_t private_data;

uint8_t sdcard_cwd(uint8_t index)
{
    if (index >= private_data.file_list_count)
    {
        return SDCARD_CWD_RESULT_BAD_INDEX;
    }

    if (!(private_data.file_entries[index].flags & AM_DIR))
    {
        return SDCARD_CWD_RESULT_NOT_DIRECTORY;
    }

    const char *nm = private_data.file_entries[index].name;
    size_t clen = strlen(private_data.cwd);
    size_t nlen = strlen(nm);
    if (clen + 1u + nlen >= sizeof(private_data.cwd))
    {
        return SDCARD_CWD_RESULT_PATH_TOO_LONG;
    }
    snprintf(private_data.cwd + clen, sizeof(private_data.cwd) - clen, "/%s", nm);
    return SDCARD_CWD_RESULT_OK;
}

uint8_t sdcard_cwd_root()
{
    private_data.cwd[0] = '0';
    private_data.cwd[1] = ':';
    private_data.cwd[2] = '\0';
    return SDCARD_CWD_RESULT_OK;
}

uint8_t sdcard_cwd_parent()
{
    size_t len = strlen(private_data.cwd);
    if (len <= 2u)
    {
        return SDCARD_CWD_RESULT_AT_ROOT;
    }

    char *slash = strrchr(private_data.cwd, '/');
    if (slash == NULL)
    {
        return SDCARD_CWD_RESULT_AT_ROOT;
    }
    *slash = '\0';

    return SDCARD_CWD_RESULT_OK;
}

#if SD_CARD_SPI_ENABLE

void sdcard_flash_sector()
{
    const uint32_t flash_offset = private_data.flash_sector_offset;
    const uint8_t *flash_existing = (const uint8_t *)(XIP_BASE + flash_offset);
    if (memcmp(flash_existing, private_data.cached_sector_buffer, SDCARD_FILE_CHUNK_SIZE) == 0)
    {
        private_data.flash_sector_result = SDCARD_FILE_RESULT_OK;
        private_data.flash_sector_ready = 1;
        return;
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offset, SDCARD_FILE_CHUNK_SIZE);
    flash_range_program(flash_offset, private_data.cached_sector_buffer, SDCARD_FILE_CHUNK_SIZE);
    restore_interrupts(ints);

    private_data.flash_sector_result = SDCARD_FILE_RESULT_OK;
    private_data.flash_sector_ready = 1;
}

void sdcard_remount()
{
    f_unmount("0:");

    FRESULT file_result = f_mount(&private_data.fatfs, "0:", 1);
    if (file_result != FR_OK)
    {
        private_data.sd_fat_mounted = 0;
        private_data.remount_result = SDCARD_FILE_RESULT_ERROR;
        private_data.remount_ready = 1;
        return;
    }
    private_data.sd_fat_mounted = 1;
    private_data.remount_result = SDCARD_FILE_RESULT_OK;
    private_data.remount_ready = 1;
}

void sdcard_dir_list()
{
    FRESULT file_result;
    DIR directory;
    FILINFO file_info;

    if (!private_data.sd_fat_mounted)
    {
        file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            private_data.file_list_result = SDCARD_FILE_RESULT_ERROR;
            private_data.file_list_ready = 1;
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    memset(&directory, 0, sizeof(directory));
    memset(&file_info, 0, sizeof(file_info));
    file_result = f_findfirst(&directory, &file_info, private_data.cwd, "*");

    uint32_t current_offset = 0;
    uint8_t file_count = 0;
    while (file_result == FR_OK && file_info.fname[0] != 0 && file_count < SDCARD_MAX_ENTRIES)
    {
        if (file_info.fname[0] == '.' && file_info.fname[1] == '\0')
        {
            file_result = f_findnext(&directory, &file_info);
            continue;
        }

        if (file_info.fname[0] == '.' && file_info.fname[1] == '.' && file_info.fname[2] == '\0')
        {
            file_result = f_findnext(&directory, &file_info);
            continue;
        }

        if (current_offset >= private_data.file_list_offset)
        {
            sdcard_file_entry_t* file_entry = &private_data.file_entries[file_count];
            file_entry->id = current_offset;
            file_entry->flags = file_info.fattrib;
            file_entry->file_size = (file_info.fattrib & AM_DIR) ? 0 : (uint32_t)file_info.fsize;
            strncpy(file_entry->name, file_info.fname, SDCARD_PATH_MAX - 1u);
            file_entry->name[SDCARD_PATH_MAX - 1u] = '\0';
            file_entry->file_name_length = (uint16_t)strlen(file_entry->name);

            file_count++;
        }

        file_result = f_findnext(&directory, &file_info);
        current_offset++;
    }

    f_closedir(&directory);
    private_data.file_list_count = file_count;
    private_data.file_list_ready = 1;
    private_data.file_list_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_open()
{
    FRESULT file_result;
    DIR directory;
    FILINFO file_info;

    f_close(&private_data.open_file);

    if (private_data.open_file_index >= private_data.file_list_count) {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_RESULT_INVALID_INDEX;
        return;
    }

    if (private_data.file_entries[private_data.open_file_index].flags  & AM_DIR)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_RESULT_IS_DIRECTORY;
        return;
    }

    if (!private_data.sd_fat_mounted)
    {
        file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            private_data.open_file_ready = 1;
            private_data.open_file_result = SDCARD_FILE_RESULT_ERROR;
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    char path[SDCARD_PATH_MAX];
    if (strcmp(private_data.cwd, "") == 0)
    {
        snprintf(path, sizeof(path), "/%s", private_data.file_entries[private_data.open_file_index].name);
    }
    else
    {
        snprintf(path, sizeof(path), "%s/%s", private_data.cwd, private_data.file_entries[private_data.open_file_index].name);
    }

    FRESULT fr = f_open(&private_data.open_file, path, FA_READ);
    if (fr != FR_OK)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_RESULT_ERROR;
        return;
    }

    private_data.open_file_size = private_data.file_entries[private_data.open_file_index].file_size;
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

uint8_t sdcard_file_close()
{
    FRESULT fr = f_close(&private_data.open_file);
    return fr == FR_OK ? SDCARD_FILE_RESULT_OK : SDCARD_FILE_RESULT_ERROR;
}

uint8_t sdcard_file_read_sector(uint32_t sector_index, uint32_t* sector_length)
{
    FRESULT file_result;

    if (!private_data.sd_fat_mounted)
    {
        file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            *sector_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }
        private_data.sd_fat_mounted = 1;
    }

    if (private_data.open_file.obj.fs == NULL)
    {
        *sector_length = 0;
        return SDCARD_FILE_RESULT_ERROR;
    }

    uint32_t max_sector_index = (private_data.open_file_size + SDCARD_FILE_CHUNK_SIZE - 1u) / SDCARD_FILE_CHUNK_SIZE;
    if (sector_index >= max_sector_index)
    {
        *sector_length = 0;
        return SDCARD_FILE_RESULT_ERROR;
    }

    if (private_data.cached_sector_index == 0xffffffff || private_data.cached_sector_index != sector_index)
    {
        uint32_t offset = sector_index * (uint32_t)SDCARD_FILE_CHUNK_SIZE;
        uint32_t remaining = private_data.open_file_size - offset;

        file_result = f_lseek(&private_data.open_file, (FSIZE_t)offset);
        if (file_result != FR_OK)
        {
            *sector_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }

        UINT bytes_read = 0;
        UINT bytes_to_read = (UINT)((remaining > SDCARD_FILE_CHUNK_SIZE) ? SDCARD_FILE_CHUNK_SIZE : remaining);
        file_result = f_read(&private_data.open_file, private_data.cached_sector_buffer, bytes_to_read, &bytes_read);
        if (file_result != FR_OK)
        {
            *sector_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }

        private_data.cached_sector_index = sector_index;
        private_data.cached_sector_length = (uint16_t)bytes_read;
    }

    *sector_length = private_data.cached_sector_length;
    return SDCARD_FILE_RESULT_OK;
}

#else /* !SD_CARD_SPI_ENABLE */

void sdcard_flash_sector(void)
{
    private_data.flash_sector_ready = 1;
    private_data.flash_sector_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_remount()
{
    private_data.remount_ready = 1;
    private_data.remount_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_dir_list()
{
    private_data.file_list_ready = 1;
    private_data.file_list_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_open()
{
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

uint8_t sdcard_file_close()
{
    return SDCARD_FILE_RESULT_OK;
}

uint8_t sdcard_file_read_sector(uint32_t sector_index, uint32_t* sector_length)
{
    (void)sector_index;
    *sector_length = 0;
    return SDCARD_FILE_RESULT_OK;
}

#endif

void sdcard_queue_command(uint8_t cmd, uint8_t data)
{
    if (cmd == SDCARD_COMMAND_NONE) {
        return;
    }
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.data.cmd = (uint8_t)cmd;
    _item.data.param1 = (uint8_t)data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

bool sdcard_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id) 
{
    uint32_t offset = (addr - lpc_mem_windows[window_id].base_addr);
    offset = offset & (lpc_mem_windows[window_id].length - 1);

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_SD_BIOS)
    {
        uint32_t mirror = offset & (private_data.open_file_size - 1u);
        uint32_t sector = mirror >> SDCARD_FILE_CHUNK_SIZE_SHIFT;
        if (private_data.cached_sector_index != 0xffffffff && private_data.cached_sector_index == sector) {
            uint32_t sector_offset = mirror & (SDCARD_FILE_CHUNK_SIZE - 1);
            *data = private_data.cached_sector_buffer[sector_offset];
            return true;
        }
        uint32_t sector_length = 0;
        sdcard_file_read_sector(sector, &sector_length);
        uint32_t sector_offset = mirror & (SDCARD_FILE_CHUNK_SIZE - 1);
        *data = sector_offset < sector_length ? private_data.cached_sector_buffer[sector_offset] : 0;
        return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_NAME)
    {
        if (private_data.file_list_name_index < private_data.file_list_count) {
            uint16_t length = private_data.file_entries[private_data.file_list_name_index].file_name_length;
            *data = offset < length ? private_data.file_entries[private_data.file_list_name_index].name[offset] : 0;
        }
		return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_SD_SECTOR)
    {
        uint32_t sector_length = 0;
        sdcard_file_read_sector(offset >> SDCARD_FILE_CHUNK_SIZE_SHIFT, &sector_length);
        uint32_t sector_offset = offset & (SDCARD_FILE_CHUNK_SIZE - 1);
        *data = sector_offset < sector_length ? private_data.cached_sector_buffer[sector_offset] : 0;
        return true;
    }

    *data = 0;
	return true;
}

bool sdcard_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) 
{
	return true;
}

uint8_t sdcard_handler_control_peek(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case SDCARD_COMMAND_RESPONSE_REMOUNT_READY:
            return private_data.remount_ready;
        case SDCARD_COMMAND_RESPONSE_REMOUNT_RESULT:
            return private_data.remount_result;

        case SDCARD_COMMAND_RESPONSE_FILE_LIST_COUNT:
            return private_data.file_list_count;
        case SDCARD_COMMAND_RESPONSE_FILE_LIST_READY:
            return private_data.file_list_ready;
        case SDCARD_COMMAND_RESPONSE_FILE_LIST_RESULT:
            return private_data.file_list_result;

        case SDCARD_COMMAND_RESPONSE_OPEN_FILE_READY:
            return private_data.open_file_ready;
        case SDCARD_COMMAND_RESPONSE_OPEN_FILE_RESULT:
            return private_data.open_file_result;

        case SDCARD_COMMAND_RESPONSE_FLASH_SECTOR_READY:
            return private_data.flash_sector_ready;
        case SDCARD_COMMAND_RESPONSE_FLASH_SECTOR_RESULT:
            return private_data.flash_sector_result;

        case SDCARD_COMMAND_CLOSE_FILE:
            return sdcard_file_close();

        case SDCARD_COMMAND_FILE_READ_SECTOR:
            return sdcard_file_read_sector(current_long_val, &current_long_val);

        case SDCARD_COMMAND_CWD_ROOT:
            return sdcard_cwd_root();
        case SDCARD_COMMAND_CWD_PARENT:
            return sdcard_cwd_parent();

        case SDCARD_COMMAND_SET_CWD_FROM_INDEX:
            return sdcard_cwd(data);
        case SDCARD_COMMAND_GET_FILE_FLAGS_FROM_INDEX:
            return data < private_data.file_list_count ? private_data.file_entries[data].flags : 0;

	}
	return 0;
}

uint8_t sdcard_handler_control_set(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case SDCARD_COMMAND_REQUEST_FLASH_SECTOR:
            private_data.flash_sector_offset = current_long_val;
            private_data.flash_sector_ready = 0;
            private_data.flash_sector_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_REMOUNT:
            private_data.remount_ready = 0;
            private_data.remount_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_FILE_LIST:
            private_data.file_list_offset = current_long_val;
            private_data.file_list_count = 0;
            private_data.file_list_name_index = 0;
            private_data.file_list_ready = 0;
            private_data.file_list_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_OPEN_FILE:
            private_data.cached_sector_index = 0xffffffff;
            private_data.open_file_index = data;
            private_data.open_file_size = 0;
            private_data.open_file_ready = 0;
            private_data.open_file_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_GET_FILE_NAME_FROM_INDEX:
            private_data.file_list_name_index = data;
            break;
        case SDCARD_COMMAND_SET_PAYLOAD_TYPE:
            private_data.payload_type = data;
            break;
        case SDCARD_COMMAND_GET_FILE_SIZE_FROM_INDEX:
            current_long_val = data < private_data.file_list_count ? private_data.file_entries[data].file_size : 0;
            break;
	}
	return 0;
}

uint8_t sdcard_handler_control(uint8_t cmd, uint8_t data, bool is_read) 
{
	return is_read ? sdcard_handler_control_peek(cmd, data) : sdcard_handler_control_set(cmd, data);
}

void sdcard_handler_poll()
{
    MODXO_QUEUE_ITEM_T _item;
    if (modxo_queue_remove(&private_data.queue, &_item))
    {
        if (_item.iscmd)
        {
            MODXO_SDCARD_CMD rx_cmd;
            rx_cmd.raw = _item.data.raw;
            switch (rx_cmd.cmd)
            {
                case SDCARD_COMMAND_REQUEST_FLASH_SECTOR:
                    sdcard_flash_sector();
                    break;
                case SDCARD_COMMAND_REQUEST_REMOUNT:
                    sdcard_remount();
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_LIST:
                    sdcard_dir_list();
                    break;
                case SDCARD_COMMAND_REQUEST_OPEN_FILE:
                    sdcard_file_open();
                    break;
                default:
                    break;
            }
        }
    }
}

void sdcard_handler_powerup() 
{
    memset(&private_data, 0, sizeof(private_data));
	sdcard_cwd_root();
    private_data.cached_sector_index = 0xffffffff;
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), SDCARD_QUEUE_BUFFER_LEN);
}