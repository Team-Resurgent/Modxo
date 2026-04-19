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

#include <modxo_pinout.h>

#if SD_CARD_SPI_ENABLE
#include "ff.h"
#endif

#define SDCARD_MAX_ENTRIES 8
#define SDCARD_QUEUE_BUFFER_LEN 1024
#define SDCARD_PATH_MAX 280
#define SDCARD_LIST_FLAG_DIRECTORY 0x01

#define SDCARD_FILE_CHUNK_SIZE 4096

#define SDCARD_CWD_RESULT_OK 0
#define SDCARD_CWD_RESULT_BAD_INDEX 1
#define SDCARD_CWD_RESULT_NOT_DIRECTORY 2
#define SDCARD_CWD_RESULT_PATH_TOO_LONG 3
#define SDCARD_CWD_RESULT_AT_ROOT 4

#define SDCARD_FILE_READ_RESULT_IDLE 0 
#define SDCARD_FILE_READ_RESULT_OK 1
#define SDCARD_FILE_READ_RESULT_ERROR 2
#define SDCARD_FILE_READ_RESULT_INVALID_INDEX 3
#define SDCARD_FILE_READ_RESULT_IS_DIRECTORY 4



#define SDCARD_COMMAND_NONE 0

#define SDCARD_COMMAND_REQUEST_FILE_LIST 1 // set long value for offset
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_COUNT 2 
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_READY 3
#define SDCARD_COMMAND_RESPONSE_FILE_LIST_RESULT 4

// need to have commands to get elements from list

#define SDCARD_COMMAND_REQUEST_OPEN_FILE 5 // data = index of list
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_READY 6
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_RESULT 7

#define SDCARD_COMMAND_CLOSE_FILE 8

#define SDCARD_COMMAND_FILE_READ_SECTOR 9 // set long value for sector index, sets long to length read

#define SDCARD_COMMAND_CWD 10
#define SDCARD_COMMAND_CWD_ROOT 11
#define SDCARD_COMMAND_CWD_PARENT 12

#define SDCARD_COMMAND_SET_FILE_LIST_SELECTED_INDEX 13
#define SDCARD_COMMAND_SET_PAYLOAD_TYPE 14

//todo get file struct






// #define SDCARD_COMMAND_SET_LIST_ENTRY_INDEX 32
// #define SDCARD_COMMAND_SET_LIST_NAME_INDEX 33
// #define SDCARD_COMMAND_SET_FILE_CHUNK_READ_INDEX 36
// #define SDCARD_COMMAND_SET_CWD_PARENT 37
// #define SDCARD_COMMAND_SET_CWD 38

// // IO Read — directory listing (current CWD)




// #define SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_FLAGS 67
// #define SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_BYTE 69

// // IO Read — file data (chunk up to SDCARD_FILE_CHUNK_SIZE bytes, file up to SDCARD_FILE_MAX_SIZE)

// #define SDCARD_COMMAND_GET_RESPONSE_FILE_OP_READY 70 
// #define SDCARD_COMMAND_GET_RESPONSE_FILE_SECTOR_SIZE 71
// #define SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE 74 
// #define SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT 78
// #define SDCARD_COMMAND_GET_RESPONSE_CWD_RESULT 79

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

    uint32_t file_list_offset;
    uint8_t file_list_count;
    uint8_t file_list_selected_index;
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


uint32_t sdcard_window_offsets[NUM_LPC_MEM_WINDOWS];

#define PAYLOAD_TYPE_NONE 0
#define PAYLOAD_TYPE_FILE_NAME 1
#define PAYLOAD_TYPE_FILE_SD_SECTOR 2
#define PAYLOAD_TYPE_FILE_SD_BIOS 3

uint8_t sdcard_cwd(uint8_t index)
{
    if (index >= private_data.file_list_count)
    {
        return SDCARD_CWD_RESULT_BAD_INDEX;
    }

    if (!(private_data.file_entries[index].flags & SDCARD_LIST_FLAG_DIRECTORY))
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
            private_data.file_list_result = SDCARD_FILE_READ_RESULT_ERROR;
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
            file_entry->flags = (file_info.fattrib & AM_DIR) ? (uint8_t)SDCARD_LIST_FLAG_DIRECTORY : 0;
            file_entry->file_size = file_entry->flags == SDCARD_LIST_FLAG_DIRECTORY ? 0 : (uint32_t)file_info.fsize;
            strncpy(file_entry->name, file_info.fname, SDCARD_PATH_MAX);
            file_entry->file_name_length = strlen(private_data.file_entries[file_count].name);

            file_count++;

            file_result = f_findnext(&directory, &file_info);
        }

        current_offset++;
    }

    f_closedir(&directory);
    private_data.file_list_count = file_count;
    private_data.file_list_ready = 1;
    private_data.file_list_result = SDCARD_FILE_READ_RESULT_OK;
}

void sdcard_file_open()
{
    FRESULT file_result;
    DIR directory;
    FILINFO file_info;

    f_close(&private_data.open_file);

    if (private_data.open_file_index >= private_data.file_list_count) {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_READ_RESULT_INVALID_INDEX;
        return;
    }

    if (private_data.file_entries[private_data.open_file_index].flags == SDCARD_LIST_FLAG_DIRECTORY)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_READ_RESULT_IS_DIRECTORY;
        return;
    }

    if (!private_data.sd_fat_mounted)
    {
        file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            private_data.open_file_ready = 1;
            private_data.open_file_result = SDCARD_FILE_READ_RESULT_ERROR;
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    char path[SDCARD_PATH_MAX];
    if (strcmp(private_data.cwd, "0:") == 0)
    {
        snprintf(path, sizeof(path), "0:/%s", private_data.file_entries[private_data.open_file_index].name);
    }
    else
    {
        snprintf(path, sizeof(path), "%s/%s", private_data.cwd, private_data.file_entries[private_data.open_file_index].name);
    }

    FRESULT fr = f_open(&private_data.open_file, path, FA_READ);
    if (fr != FR_OK)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_READ_RESULT_ERROR;
        return;
    }

    private_data.open_file_size = private_data.file_entries[private_data.open_file_index].file_size;
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_READ_RESULT_OK;
}

uint8_t sdcard_file_close()
{
    f_close(&private_data.open_file);
    return SDCARD_FILE_READ_RESULT_OK;
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
            return SDCARD_FILE_READ_RESULT_ERROR;
        }
        private_data.sd_fat_mounted = 1;
    }

    if (private_data.open_file.obj.fs == NULL)
    {
        *sector_length = 0;
        return SDCARD_FILE_READ_RESULT_ERROR;
    }

    uint32_t max_sector_index = (private_data.open_file_size + SDCARD_FILE_CHUNK_SIZE - 1u) / SDCARD_FILE_CHUNK_SIZE;
    if (sector_index > max_sector_index)
    {
        *sector_length = 0;
        return SDCARD_FILE_READ_RESULT_ERROR;
    }

    if (private_data.cached_sector_index = 0xffffffff || private_data.cached_sector_index != sector_index)
    {
        uint32_t offset = sector_index * (uint32_t)SDCARD_FILE_CHUNK_SIZE;
        uint32_t remaining = private_data.open_file_size - offset;

        file_result = f_lseek(&private_data.open_file, (FSIZE_t)offset);
        if (file_result != FR_OK)
        {
            *sector_length = 0;
            return SDCARD_FILE_READ_RESULT_ERROR;
        }

        UINT bytes_read = 0;
        UINT bytes_to_read = (UINT)((remaining > SDCARD_FILE_CHUNK_SIZE) ? SDCARD_FILE_CHUNK_SIZE : remaining);
        file_result = f_read(&private_data.open_file, private_data.cached_sector_buffer, bytes_to_read, &bytes_read);
        if (file_result != FR_OK)
        {
            *sector_length = 0;
            return SDCARD_FILE_READ_RESULT_ERROR;
        }

        private_data.cached_sector_index = sector_index;
        private_data.cached_sector_length = (uint16_t)bytes_read;
    }

    *sector_length = private_data.cached_sector_length;
    return SDCARD_FILE_READ_RESULT_OK;
}

#else /* !SD_CARD_SPI_ENABLE */

void sdcard_dir_list()
{
    private_data.file_list_ready = 1;
    private_data.file_list_result = SDCARD_FILE_READ_RESULT_OK
}

void sdcard_file_open()
{
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_READ_RESULT_OK;
}

uint8_t sdcard_file_close()
{
    return SDCARD_FILE_READ_RESULT_OK;
}

uint8_t sdcard_file_read_sector(uint32_t sectot_index, uint32_t* sector_length)
{
    (void*)sectot_index;
    *sector_length = 0;
    return SDCARD_FILE_READ_RESULT_OK;
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

    if (private_data.payload_type == PAYLOAD_TYPE_FILE_NAME)
    {
        if (private_data.file_list_selected_index < private_data.file_list_count) {
            uint16_t length = private_data.file_entries[private_data.file_list_selected_index].file_name_length;
            *data = offset < length ? private_data.file_entries[private_data.file_list_selected_index].name[offset] : 0;
            return true;
        }
    }
    else if (private_data.payload_type == PAYLOAD_TYPE_FILE_SD_SECTOR)
    {
        uint32_t sector = offset / SDCARD_FILE_CHUNK_SIZE;
        uint32_t sector_length = 0;
        sdcard_file_read_sector(sector, &sector_length);
        uint32_t sector_offset = offset & (SDCARD_FILE_CHUNK_SIZE - 1);
        *data = sector_offset < sector_length ? private_data.cached_sector_buffer[sector_offset] : 0;
        return true;
    }
    else if (private_data.payload_type == PAYLOAD_TYPE_FILE_SD_BIOS)
    {
        uint32_t mirror = offset & ((256 * 1024) - 1); // change this to bios size
        uint32_t sector = mirror / SDCARD_FILE_CHUNK_SIZE;
        uint32_t sector_length = 0;
        sdcard_file_read_sector(sector, &sector_length);
        uint32_t sector_offset = mirror & (SDCARD_FILE_CHUNK_SIZE - 1);
        *data = private_data.cached_sector_buffer[sector_offset];
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

        case SDCARD_COMMAND_CLOSE_FILE:
            return sdcard_file_close();

        case SDCARD_COMMAND_FILE_READ_SECTOR:
            return sdcard_file_read_sector(current_long_val, &current_long_val);

        case SDCARD_COMMAND_CWD:
            return sdcard_cwd(data);
        case SDCARD_COMMAND_CWD_ROOT:
            return sdcard_cwd_root();
        case SDCARD_COMMAND_CWD_PARENT:
            return sdcard_cwd_parent();


        // case SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_ID:
        //     if (private_data.root_list_entry_sel < private_data.file_list_count)
        //     {
        //         return private_data.file_entries[private_data.root_list_entry_sel].id;
        //     }
        //     break;
        // case SDCARD_COMMAND_GET_RESPONSE_LIST_ENTRY_FLAGS:
        //     if (private_data.root_list_entry_sel < private_data.file_list_count)
        //     {
        //         return private_data.file_entries[private_data.root_list_entry_sel].flags;
        //     }
        //     break;
	}
	return 0;
}

uint8_t sdcard_handler_control_set(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case SDCARD_COMMAND_REQUEST_FILE_LIST:
            private_data.file_list_offset = current_long_val;
            private_data.file_list_count = 0;
            private_data.file_list_selected_index = 0;
            private_data.file_list_ready = 0;
            private_data.file_list_result = SDCARD_FILE_READ_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_OPEN_FILE:
            private_data.cached_sector_index = 0xffffffff;
            private_data.open_file_index = data;
            private_data.open_file_size = 0;
            private_data.open_file_ready = 0;
            private_data.open_file_result = SDCARD_FILE_READ_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_SET_FILE_LIST_SELECTED_INDEX:
            private_data.file_list_selected_index = data;
            break;
        case SDCARD_COMMAND_SET_PAYLOAD_TYPE:
            private_data.payload_type = data;
            break;


        // case SDCARD_COMMAND_GET_RESPONSE_LIST_NAME_LENGTH:
        //     if (private_data.root_list_entry_sel < private_data.root_list_count)
        //     {
        //         current_long_val = strlen(private_data.root_entries[private_data.root_list_entry_sel].name);
        //     }
        //     break;
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
    sdcard_cwd_root();

    memset(&private_data, 0, sizeof(private_data));
    private_data.cached_sector_index = 0xffffffff;
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), SDCARD_QUEUE_BUFFER_LEN);
}