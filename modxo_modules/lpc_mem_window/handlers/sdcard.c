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
#include "diskio.h"
#endif

#define SDCARD_DISK_SECTOR_SIZE 512

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

#define SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_INDEX 8 // data = index of list
#define SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_PATH 9
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_READY 10
#define SDCARD_COMMAND_RESPONSE_OPEN_FILE_RESULT 11

#define SDCARD_COMMAND_REQUEST_FLASH_CHUNK 12 // flashes loaded chunk at current_long offset
#define SDCARD_COMMAND_RESPONSE_FLASH_CHUNK_READY 13
#define SDCARD_COMMAND_RESPONSE_FLASH_CHUNK_RESULT 14

#define SDCARD_COMMAND_CLOSE_FILE 15

#define SDCARD_COMMAND_FILE_READ_CHUNK 16 // peek: current_long_val = chunk index; returns bytes read in long

#define SDCARD_COMMAND_CWD_ROOT 17
#define SDCARD_COMMAND_CWD_PARENT 18
#define SDCARD_COMMAND_SET_CWD_FROM_INDEX 19

#define SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_INDEX 20
#define SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_PATH 21
#define SDCARD_COMMAND_RESPONSE_FILE_INFO_READY 22
#define SDCARD_COMMAND_RESPONSE_FILE_INFO_RESULT 23
#define SDCARD_COMMAND_GET_FILE_INFO_SIZE 24
#define SDCARD_COMMAND_GET_FILE_INFO_FLAGS 25
#define SDCARD_COMMAND_GET_FILE_INFO_MODIFIED 26
#define SDCARD_COMMAND_GET_FILE_INFO_CREATED 27

#define SDCARD_COMMAND_REQUEST_VOLUME_SPACE 28
#define SDCARD_COMMAND_RESPONSE_VOLUME_SPACE_READY 29
#define SDCARD_COMMAND_RESPONSE_VOLUME_SPACE_RESULT 30
#define SDCARD_COMMAND_GET_VOLUME_FREE 31
#define SDCARD_COMMAND_GET_VOLUME_TOTAL 32

#define SDCARD_COMMAND_REQUEST_DELETE_FROM_PATH 33
#define SDCARD_COMMAND_RESPONSE_PATH_DELETE_READY 34
#define SDCARD_COMMAND_RESPONSE_PATH_DELETE_RESULT 35

#define SDCARD_COMMAND_REQUEST_CREATE_DIR_FROM_PATH 36
#define SDCARD_COMMAND_REQUEST_CREATE_FILE_FROM_PATH 37 // empty file; size grows via write chunks
#define SDCARD_COMMAND_RESPONSE_PATH_CREATE_READY 38
#define SDCARD_COMMAND_RESPONSE_PATH_CREATE_RESULT 39

#define SDCARD_COMMAND_SET_FILE_WRITE_CHUNK_SIZE 40 // current_long_val = byte count
#define SDCARD_COMMAND_REQUEST_WRITE_FILE_CHUNK 41 // current_long_val = chunk index
#define SDCARD_COMMAND_RESPONSE_FILE_WRITE_READY 42
#define SDCARD_COMMAND_RESPONSE_FILE_WRITE_RESULT 43

#define SDCARD_COMMAND_GET_DISK_SECTOR_COUNT 45 // set: data 0/1 = low/high dword of sector count
#define SDCARD_COMMAND_REQUEST_DISK_READ_SECTOR 46
#define SDCARD_COMMAND_RESPONSE_DISK_READ_READY 47
#define SDCARD_COMMAND_RESPONSE_DISK_READ_RESULT 48
#define SDCARD_COMMAND_REQUEST_DISK_WRITE_SECTOR 49
#define SDCARD_COMMAND_RESPONSE_DISK_WRITE_READY 50
#define SDCARD_COMMAND_RESPONSE_DISK_WRITE_RESULT 51

#define SDCARD_COMMAND_SET_PAYLOAD_TYPE 52

#define SDCARD_VOLUME_SPACE_DWORD_LOW 0
#define SDCARD_VOLUME_SPACE_DWORD_HIGH 1

#define SDCARD_DISK_SECTOR_DWORD_LOW 0
#define SDCARD_DISK_SECTOR_DWORD_HIGH 1

#define SDCARD_PAYLOAD_TYPE_NONE 0
#define SDCARD_PAYLOAD_TYPE_FILE_NAME 1
#define SDCARD_PAYLOAD_TYPE_FILE_SD_CHUNK 2
#define SDCARD_PAYLOAD_TYPE_FILE_SD_BIOS 3
#define SDCARD_PAYLOAD_TYPE_CWD 4
#define SDCARD_PAYLOAD_TYPE_PATH 5
#define SDCARD_PAYLOAD_TYPE_DISK_SECTOR 6

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

    uint32_t flash_chunk_offset;
    uint8_t flash_chunk_ready;
    uint8_t flash_chunk_result;

    uint32_t file_list_offset;
    uint8_t file_list_count;
    uint8_t file_list_ready;
    uint8_t file_list_result;
    sdcard_file_entry_t file_entries[SDCARD_MAX_ENTRIES];

    uint8_t open_file_index;
    uint8_t open_file_ready;
    uint8_t open_file_result;
#if SD_CARD_SPI_ENABLE
    FIL open_file;
#endif

    uint32_t cached_chunk_index;
    uint32_t cached_chunk_length;
    uint8_t cached_chunk_buffer[SDCARD_FILE_CHUNK_SIZE];

    char cwd[SDCARD_PATH_MAX];
    char path[SDCARD_PATH_MAX];

    uint8_t file_info_index;
    uint8_t file_info_ready;
    uint8_t file_info_result;
    uint8_t file_info_flags;
    uint32_t file_info_size;
    char file_info_name[SDCARD_PATH_MAX];
    uint16_t file_info_name_length;
    uint32_t file_info_modified;
    uint32_t file_info_created;

    uint8_t volume_space_ready;
    uint8_t volume_space_result;
    uint64_t volume_free_bytes;
    uint64_t volume_total_bytes;

    uint8_t path_create_ready;
    uint8_t path_create_result;

    uint8_t path_delete_ready;
    uint8_t path_delete_result;

    uint32_t write_chunk_index;
    uint32_t write_chunk_size;
    uint8_t file_write_ready;
    uint8_t file_write_result;

    uint64_t disk_num_sectors;
    uint32_t disk_sector_index;
    uint8_t disk_sector_buffer[SDCARD_DISK_SECTOR_SIZE];
    uint8_t disk_read_ready;
    uint8_t disk_read_result;
    uint8_t disk_write_ready;
    uint8_t disk_write_result;

    uint8_t payload_type;

#if SD_CARD_SPI_ENABLE
    FATFS fatfs;
    uint8_t sd_fat_mounted;
#endif
} sdcard_state_t;

static sdcard_state_t private_data;

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

void sdcard_flash_chunk()
{
    const uint32_t flash_offset = private_data.flash_chunk_offset;
    const uint8_t *flash_existing = (const uint8_t *)(XIP_BASE + flash_offset);
    if (memcmp(flash_existing, private_data.cached_chunk_buffer, SDCARD_FILE_CHUNK_SIZE) == 0)
    {
        private_data.flash_chunk_result = SDCARD_FILE_RESULT_OK;
        private_data.flash_chunk_ready = 1;
        return;
    }

    uint32_t ints = save_and_disable_interrupts();
    flash_range_erase(flash_offset, SDCARD_FILE_CHUNK_SIZE);
    flash_range_program(flash_offset, private_data.cached_chunk_buffer, SDCARD_FILE_CHUNK_SIZE);
    restore_interrupts(ints);

    private_data.flash_chunk_result = SDCARD_FILE_RESULT_OK;
    private_data.flash_chunk_ready = 1;
}

void sdcard_volume_space()
{
    DWORD fre_clust;
    FATFS *fs;

    if (!private_data.sd_fat_mounted)
    {
        FRESULT file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            private_data.volume_space_result = SDCARD_FILE_RESULT_ERROR;
            private_data.volume_space_ready = 1;
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    if (f_getfree("0:", &fre_clust, &fs) != FR_OK)
    {
        private_data.volume_free_bytes = 0;
        private_data.volume_total_bytes = 0;
        private_data.volume_space_result = SDCARD_FILE_RESULT_ERROR;
        private_data.volume_space_ready = 1;
        return;
    }

    uint64_t cluster_bytes = (uint64_t)fs->csize * 512u;
    private_data.volume_total_bytes = (uint64_t)(fs->n_fatent - 2) * cluster_bytes;
    private_data.volume_free_bytes = (uint64_t)fre_clust * cluster_bytes;
    private_data.volume_space_result = SDCARD_FILE_RESULT_OK;
    private_data.volume_space_ready = 1;
}

static bool sdcard_disk_refresh_sector_count(void)
{
    LBA_t count = 0;

    if (disk_ioctl(0, GET_SECTOR_COUNT, &count) != RES_OK || count == 0)
    {
        private_data.disk_num_sectors = 0;
        return false;
    }

    private_data.disk_num_sectors = (uint64_t)count;
    return true;
}

void sdcard_disk_read_sector()
{
    if (!sdcard_disk_refresh_sector_count())
    {
        private_data.disk_read_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_read_ready = 1;
        return;
    }

    if ((uint64_t)private_data.disk_sector_index >= private_data.disk_num_sectors)
    {
        private_data.disk_read_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_read_ready = 1;
        return;
    }

    if (disk_read(0, private_data.disk_sector_buffer, (LBA_t)private_data.disk_sector_index, 1) != RES_OK)
    {
        private_data.disk_read_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_read_ready = 1;
        return;
    }

    private_data.disk_read_result = SDCARD_FILE_RESULT_OK;
    private_data.disk_read_ready = 1;
}

void sdcard_disk_write_sector()
{
    if (!sdcard_disk_refresh_sector_count())
    {
        private_data.disk_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_write_ready = 1;
        return;
    }

    if ((uint64_t)private_data.disk_sector_index >= private_data.disk_num_sectors)
    {
        private_data.disk_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_write_ready = 1;
        return;
    }

    disk_ioctl(0, CTRL_SYNC, NULL);

    if (disk_write(0, private_data.disk_sector_buffer, (LBA_t)private_data.disk_sector_index, 1) != RES_OK)
    {
        private_data.disk_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.disk_write_ready = 1;
        return;
    }

    disk_ioctl(0, CTRL_SYNC, NULL);
    private_data.disk_write_result = SDCARD_FILE_RESULT_OK;
    private_data.disk_write_ready = 1;
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

static uint32_t sdcard_pack_fatfs_datetime(WORD fdate, WORD ftime)
{
    return ((uint32_t)fdate << 16) | (uint32_t)ftime;
}

static void sdcard_file_info_fail(void)
{
    private_data.file_info_flags = 0;
    private_data.file_info_size = 0;
    private_data.file_info_name[0] = '\0';
    private_data.file_info_name_length = 0;
    private_data.file_info_modified = 0;
    private_data.file_info_created = 0;
    private_data.file_info_result = SDCARD_FILE_RESULT_ERROR;
    private_data.file_info_ready = 1;
}

static void sdcard_file_info_apply(const FILINFO *file_info, uint32_t created_packed)
{
    private_data.file_info_flags = file_info->fattrib;
    private_data.file_info_size = (file_info->fattrib & AM_DIR) ? 0 : (uint32_t)file_info->fsize;
    strncpy(private_data.file_info_name, file_info->fname, SDCARD_PATH_MAX - 1u);
    private_data.file_info_name[SDCARD_PATH_MAX - 1u] = '\0';
    private_data.file_info_name_length = (uint16_t)strlen(private_data.file_info_name);
    private_data.file_info_modified = sdcard_pack_fatfs_datetime(file_info->fdate, file_info->ftime);
    private_data.file_info_created = created_packed;
    private_data.file_info_result = SDCARD_FILE_RESULT_OK;
    private_data.file_info_ready = 1;
}

static void sdcard_file_info_stat(const char *path)
{
    FILINFO file_info;

    if (!private_data.sd_fat_mounted)
    {
        FRESULT file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            sdcard_file_info_fail();
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    memset(&file_info, 0, sizeof(file_info));
    DWORD crtime = 0;
    if (f_stat_ex(path, &file_info, &crtime) != FR_OK)
    {
        sdcard_file_info_fail();
        return;
    }

    sdcard_file_info_apply(&file_info, (uint32_t)crtime);
}

static void sdcard_build_entry_path(uint8_t index, char *path, size_t path_size)
{
    if (strcmp(private_data.cwd, "") == 0)
    {
        snprintf(path, path_size, "/%s", private_data.file_entries[index].name);
    }
    else
    {
        snprintf(path, path_size, "%s/%s", private_data.cwd, private_data.file_entries[index].name);
    }
}

void sdcard_file_open_from_index()
{
    FRESULT file_result;

    f_close(&private_data.open_file);
    private_data.cached_chunk_index = 0xffffffff;

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
    sdcard_build_entry_path(private_data.open_file_index, path, sizeof(path));

    FRESULT fr = f_open(&private_data.open_file, path, FA_READ | FA_WRITE);
    if (fr != FR_OK)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_RESULT_ERROR;
        return;
    }

    sdcard_file_info_stat(path);
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_open_from_path()
{
    FRESULT file_result;

    f_close(&private_data.open_file);
    private_data.cached_chunk_index = 0xffffffff;

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

    FRESULT fr = f_open(&private_data.open_file, private_data.path, FA_READ | FA_WRITE);
    if (fr != FR_OK)
    {
        private_data.open_file_ready = 1;
        private_data.open_file_result = SDCARD_FILE_RESULT_ERROR;
        return;
    }

    sdcard_file_info_stat(private_data.path);
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

static bool sdcard_ensure_mounted(void)
{
    if (private_data.sd_fat_mounted)
    {
        return true;
    }
    if (f_mount(&private_data.fatfs, "0:", 1) != FR_OK)
    {
        return false;
    }
    private_data.sd_fat_mounted = 1;
    return true;
}

void sdcard_create_dir_from_path()
{
    if (!sdcard_ensure_mounted())
    {
        private_data.path_create_result = SDCARD_FILE_RESULT_ERROR;
        private_data.path_create_ready = 1;
        return;
    }

    FRESULT fr = f_mkdir(private_data.path);
    private_data.path_create_result = (fr == FR_OK) ? SDCARD_FILE_RESULT_OK : SDCARD_FILE_RESULT_ERROR;
    private_data.path_create_ready = 1;
}

void sdcard_delete_from_path()
{
    if (!sdcard_ensure_mounted())
    {
        private_data.path_delete_result = SDCARD_FILE_RESULT_ERROR;
        private_data.path_delete_ready = 1;
        return;
    }

    FRESULT fr = f_unlink(private_data.path);
    private_data.path_delete_result = (fr == FR_OK) ? SDCARD_FILE_RESULT_OK : SDCARD_FILE_RESULT_ERROR;
    private_data.path_delete_ready = 1;
}

void sdcard_create_file_from_path()
{
    if (!sdcard_ensure_mounted())
    {
        private_data.path_create_result = SDCARD_FILE_RESULT_ERROR;
        private_data.path_create_ready = 1;
        return;
    }

    f_close(&private_data.open_file);
    private_data.cached_chunk_index = 0xffffffff;

    FRESULT fr = f_open(&private_data.open_file, private_data.path, FA_READ | FA_WRITE | FA_CREATE_ALWAYS);
    if (fr != FR_OK)
    {
        private_data.path_create_result = SDCARD_FILE_RESULT_ERROR;
        private_data.path_create_ready = 1;
        return;
    }

    sdcard_file_info_stat(private_data.path);
    if (private_data.file_info_result != SDCARD_FILE_RESULT_OK)
    {
        private_data.file_info_size = 0;
        private_data.file_info_ready = 1;
        private_data.file_info_result = SDCARD_FILE_RESULT_OK;
    }

    private_data.path_create_result = SDCARD_FILE_RESULT_OK;
    private_data.path_create_ready = 1;
}

void sdcard_file_info_from_path()
{
    sdcard_file_info_stat(private_data.path);
}

void sdcard_file_info_from_index()
{
    if (private_data.file_info_index >= private_data.file_list_count)
    {
        private_data.file_info_result = SDCARD_FILE_RESULT_INVALID_INDEX;
        private_data.file_info_ready = 1;
        return;
    }

    char path[SDCARD_PATH_MAX];
    sdcard_build_entry_path(private_data.file_info_index, path, sizeof(path));
    sdcard_file_info_stat(path);
}

uint8_t sdcard_file_close()
{
    FRESULT fr = f_close(&private_data.open_file);
    private_data.cached_chunk_index = 0xffffffff;
    return fr == FR_OK ? SDCARD_FILE_RESULT_OK : SDCARD_FILE_RESULT_ERROR;
}

uint8_t sdcard_file_read_chunk(uint32_t chunk_index, uint32_t* chunk_length)
{
    FRESULT file_result;

    if (!private_data.sd_fat_mounted)
    {
        file_result = f_mount(&private_data.fatfs, "0:", 1);
        if (file_result != FR_OK)
        {
            *chunk_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }
        private_data.sd_fat_mounted = 1;
    }

    if (private_data.open_file.obj.fs == NULL)
    {
        *chunk_length = 0;
        return SDCARD_FILE_RESULT_ERROR;
    }

    uint32_t max_chunk_index = (private_data.file_info_size + SDCARD_FILE_CHUNK_SIZE - 1u) / SDCARD_FILE_CHUNK_SIZE;
    if (chunk_index >= max_chunk_index)
    {
        *chunk_length = 0;
        return SDCARD_FILE_RESULT_ERROR;
    }

    if (private_data.cached_chunk_index == 0xffffffff || private_data.cached_chunk_index != chunk_index)
    {
        uint32_t offset = chunk_index * (uint32_t)SDCARD_FILE_CHUNK_SIZE;
        uint32_t remaining = private_data.file_info_size - offset;

        file_result = f_lseek(&private_data.open_file, (FSIZE_t)offset);
        if (file_result != FR_OK)
        {
            *chunk_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }

        UINT bytes_read = 0;
        UINT bytes_to_read = (UINT)((remaining > SDCARD_FILE_CHUNK_SIZE) ? SDCARD_FILE_CHUNK_SIZE : remaining);
        file_result = f_read(&private_data.open_file, private_data.cached_chunk_buffer, bytes_to_read, &bytes_read);
        if (file_result != FR_OK)
        {
            *chunk_length = 0;
            return SDCARD_FILE_RESULT_ERROR;
        }

        private_data.cached_chunk_index = chunk_index;
        private_data.cached_chunk_length = (uint16_t)bytes_read;
    }

    *chunk_length = private_data.cached_chunk_length;
    return SDCARD_FILE_RESULT_OK;
}

static void sdcard_invalidate_read_cache_for_range(uint32_t file_offset, uint32_t length)
{
    if (private_data.cached_chunk_index == 0xffffffff)
    {
        return;
    }

    uint32_t range_end = file_offset + length;
    uint32_t cache_start = private_data.cached_chunk_index * (uint32_t)SDCARD_FILE_CHUNK_SIZE;
    uint32_t cache_end = cache_start + (uint32_t)SDCARD_FILE_CHUNK_SIZE;

    if (file_offset < cache_end && range_end > cache_start)
    {
        private_data.cached_chunk_index = 0xffffffff;
    }
}

void sdcard_file_write_chunk()
{
    if (!sdcard_ensure_mounted())
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    if (private_data.open_file.obj.fs == NULL || !(private_data.open_file.flag & FA_WRITE))
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    uint32_t chunk_index = private_data.write_chunk_index;
    uint32_t length = private_data.write_chunk_size;
    uint32_t file_offset = chunk_index * (uint32_t)SDCARD_FILE_CHUNK_SIZE;

    if (length == 0 || length > SDCARD_FILE_CHUNK_SIZE)
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    FRESULT file_result = f_lseek(&private_data.open_file, (FSIZE_t)file_offset);
    if (file_result != FR_OK)
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    UINT bytes_written = 0;
    file_result = f_write(&private_data.open_file, private_data.cached_chunk_buffer, (UINT)length, &bytes_written);
    if (file_result != FR_OK || bytes_written != length)
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    file_result = f_sync(&private_data.open_file);
    if (file_result != FR_OK)
    {
        private_data.file_write_result = SDCARD_FILE_RESULT_ERROR;
        private_data.file_write_ready = 1;
        return;
    }

    sdcard_invalidate_read_cache_for_range(file_offset, length);

    uint32_t new_size = file_offset + length;
    if (new_size > private_data.file_info_size)
    {
        private_data.file_info_size = new_size;
    }

    private_data.file_write_result = SDCARD_FILE_RESULT_OK;
    private_data.file_write_ready = 1;
}

#else /* !SD_CARD_SPI_ENABLE */

uint8_t sdcard_cwd(uint8_t index)
{
    (void)index;
    return SDCARD_CWD_RESULT_OK;
}

void sdcard_flash_chunk(void)
{
    private_data.flash_chunk_ready = 1;
    private_data.flash_chunk_result = SDCARD_FILE_RESULT_OK;
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

void sdcard_file_open_from_index()
{
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_open_from_path()
{
    private_data.open_file_ready = 1;
    private_data.open_file_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_info_from_path()
{
    private_data.file_info_ready = 1;
    private_data.file_info_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_info_from_index()
{
    private_data.file_info_ready = 1;
    private_data.file_info_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_volume_space()
{
    private_data.volume_space_ready = 1;
    private_data.volume_space_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_disk_refresh_sector_count()
{
    private_data.disk_refresh_sector_count_ready = 1;
    private_data.disk_refresh_sector_count_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_create_dir_from_path()
{
    private_data.path_create_ready = 1;
    private_data.path_create_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_delete_from_path()
{
    private_data.path_create_ready = 1;
    private_data.path_create_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_create_file_from_path()
{
    private_data.path_create_ready = 1;
    private_data.path_create_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_file_write_chunk()
{
    private_data.file_write_ready = 1;
    private_data.file_write_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_disk_write_sector()
{
    private_data.disk_write_ready = 1;
    private_data.disk_write_result = SDCARD_FILE_RESULT_OK;
}

void sdcard_disk_read_sector()
{
    private_data.disk_read_ready = 1;
    private_data.disk_read_result = SDCARD_FILE_RESULT_OK;
}

uint8_t sdcard_file_close()
{
    return SDCARD_FILE_RESULT_OK;
}

uint8_t sdcard_file_read_chunk(uint32_t chunk_index, uint32_t* chunk_length)
{
    (void)chunk_index;
    *chunk_length = 0;
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
        uint32_t mirror = offset & (private_data.file_info_size - 1u);
        uint32_t chunk = mirror >> SDCARD_FILE_CHUNK_SIZE_SHIFT;
        if (private_data.cached_chunk_index != 0xffffffff && private_data.cached_chunk_index == chunk)
        {
            uint32_t chunk_offset = mirror & (SDCARD_FILE_CHUNK_SIZE - 1);
            *data = chunk_offset < private_data.cached_chunk_length
                ? private_data.cached_chunk_buffer[chunk_offset]
                : 0;
            return true;
        }
        uint32_t chunk_length = 0;
        if (sdcard_file_read_chunk(chunk, &chunk_length) != SDCARD_FILE_RESULT_OK)
        {
            *data = 0;
            return true;
        }
        uint32_t chunk_offset = mirror & (SDCARD_FILE_CHUNK_SIZE - 1);
        *data = chunk_offset < chunk_length ? private_data.cached_chunk_buffer[chunk_offset] : 0;
        return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_NAME)
    {
        *data = offset < private_data.file_info_name_length ? private_data.file_info_name[offset] : 0;
		return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_SD_CHUNK)
    {
        uint32_t chunk_offset = offset & (SDCARD_FILE_CHUNK_SIZE - 1);
        if (private_data.cached_chunk_index == 0xffffffff)
        {
            *data = 0;
            return true;
        }
        *data = chunk_offset < private_data.cached_chunk_length ? private_data.cached_chunk_buffer[chunk_offset] : 0;
        return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_DISK_SECTOR)
    {
        *data =offset < SDCARD_DISK_SECTOR_SIZE ? private_data.disk_sector_buffer[offset] : 0;
        return true;
    }

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_CWD)
    {
        *data = offset < SDCARD_PATH_MAX ? private_data.cwd[offset] : 0;
        return true;
    }
    else if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_PATH)
    {
        *data = offset < SDCARD_PATH_MAX ? private_data.path[offset] : 0;
        return true;
    }

    *data = 0;
	return true;
}

bool sdcard_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id) 
{
    uint32_t offset = (addr - lpc_mem_windows[window_id].base_addr);
    offset = offset & (lpc_mem_windows[window_id].length - 1);

    if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_CWD)
    {
        if (offset < SDCARD_PATH_MAX)
        {
            private_data.cwd[offset] = *data;
            return true;
        }
    }
    else if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_PATH)
    {
        if (offset < SDCARD_PATH_MAX)
        {
            private_data.path[offset] = *data;
            return true;
        }
    }
    else if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_FILE_SD_CHUNK)
    {
        if (offset < SDCARD_FILE_CHUNK_SIZE)
        {
            private_data.cached_chunk_buffer[offset] = *data;
            return true;
        }
    }
    else if (private_data.payload_type == SDCARD_PAYLOAD_TYPE_DISK_SECTOR)
    {
        if (offset < SDCARD_DISK_SECTOR_SIZE)
        {
            private_data.disk_sector_buffer[offset] = *data;
            return true;
        }
    }

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

        case SDCARD_COMMAND_RESPONSE_FLASH_CHUNK_READY:
            return private_data.flash_chunk_ready;
        case SDCARD_COMMAND_RESPONSE_FLASH_CHUNK_RESULT:
            return private_data.flash_chunk_result;

        case SDCARD_COMMAND_RESPONSE_FILE_INFO_READY:
            return private_data.file_info_ready;
        case SDCARD_COMMAND_RESPONSE_FILE_INFO_RESULT:
            return private_data.file_info_result;

        case SDCARD_COMMAND_RESPONSE_VOLUME_SPACE_READY:
            return private_data.volume_space_ready;
        case SDCARD_COMMAND_RESPONSE_VOLUME_SPACE_RESULT:
            return private_data.volume_space_result;

        case SDCARD_COMMAND_RESPONSE_PATH_CREATE_READY:
            return private_data.path_create_ready;
        case SDCARD_COMMAND_RESPONSE_PATH_CREATE_RESULT:
            return private_data.path_create_result;

        case SDCARD_COMMAND_RESPONSE_FILE_WRITE_READY:
            return private_data.file_write_ready;
        case SDCARD_COMMAND_RESPONSE_FILE_WRITE_RESULT:
            return private_data.file_write_result;

        case SDCARD_COMMAND_RESPONSE_DISK_READ_READY:
            return private_data.disk_read_ready;
        case SDCARD_COMMAND_RESPONSE_DISK_READ_RESULT:
            return private_data.disk_read_result;

        case SDCARD_COMMAND_RESPONSE_DISK_WRITE_READY:
            return private_data.disk_write_ready;
        case SDCARD_COMMAND_RESPONSE_DISK_WRITE_RESULT:
            return private_data.disk_write_result;

        case SDCARD_COMMAND_CLOSE_FILE:
            return sdcard_file_close();

        case SDCARD_COMMAND_FILE_READ_CHUNK:
            return sdcard_file_read_chunk(current_long_val, &current_long_val);

        case SDCARD_COMMAND_CWD_ROOT:
            return sdcard_cwd_root();
        case SDCARD_COMMAND_CWD_PARENT:
            return sdcard_cwd_parent();
        case SDCARD_COMMAND_SET_CWD_FROM_INDEX:
            return sdcard_cwd(data);

        case SDCARD_COMMAND_GET_FILE_INFO_FLAGS:
            return private_data.file_info_ready ? private_data.file_info_flags : 0;

	}
	return 0;
}

uint8_t sdcard_handler_control_set(uint8_t cmd, uint8_t data) 
{
	switch(cmd) 
	{
        case SDCARD_COMMAND_REQUEST_FLASH_CHUNK:
            private_data.flash_chunk_offset = current_long_val;
            private_data.flash_chunk_ready = 0;
            private_data.flash_chunk_result = SDCARD_FILE_RESULT_IDLE;
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
            private_data.file_list_ready = 0;
            private_data.file_list_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_INDEX:
            private_data.cached_chunk_index = 0xffffffff;
            private_data.open_file_index = data;
            private_data.file_info_ready = 0;
            private_data.open_file_ready = 0;
            private_data.open_file_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_PATH:
            private_data.cached_chunk_index = 0xffffffff;
            private_data.file_info_ready = 0;
            private_data.open_file_ready = 0;
            private_data.open_file_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_SET_PAYLOAD_TYPE:
            private_data.payload_type = data;
            break;
        case SDCARD_COMMAND_GET_FILE_INFO_SIZE:
            current_long_val = private_data.file_info_ready ? private_data.file_info_size : 0;
            break;
        case SDCARD_COMMAND_GET_FILE_INFO_MODIFIED:
            current_long_val = private_data.file_info_ready ? private_data.file_info_modified : 0;
            break;
        case SDCARD_COMMAND_GET_FILE_INFO_CREATED:
            current_long_val = private_data.file_info_ready ? private_data.file_info_created : 0;
            break;
        case SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_INDEX:
            private_data.file_info_index = data;
            private_data.file_info_ready = 0;
            private_data.file_info_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_PATH:
            private_data.file_info_ready = 0;
            private_data.file_info_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_VOLUME_SPACE:
            private_data.volume_space_ready = 0;
            private_data.volume_space_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_DELETE_FROM_PATH:
            private_data.path_delete_ready = 0;
            private_data.path_delete_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_CREATE_DIR_FROM_PATH:
            private_data.path_create_ready = 0;
            private_data.path_create_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_CREATE_FILE_FROM_PATH:
            private_data.file_info_ready = 0;
            private_data.path_create_ready = 0;
            private_data.path_create_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_SET_FILE_WRITE_CHUNK_SIZE:
            private_data.write_chunk_size = current_long_val;
            break;
        case SDCARD_COMMAND_REQUEST_WRITE_FILE_CHUNK:
            private_data.write_chunk_index = current_long_val;
            private_data.file_write_ready = 0;
            private_data.file_write_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_DISK_READ_SECTOR:
            private_data.disk_sector_index = current_long_val;
            private_data.disk_read_ready = 0;
            private_data.disk_read_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_REQUEST_DISK_WRITE_SECTOR:
            private_data.disk_sector_index = current_long_val;
            private_data.disk_write_ready = 0;
            private_data.disk_write_result = SDCARD_FILE_RESULT_IDLE;
            sdcard_queue_command(cmd, data);
            break;
        case SDCARD_COMMAND_GET_VOLUME_FREE:
            current_long_val = (uint32_t)((data == SDCARD_VOLUME_SPACE_DWORD_HIGH) ? (private_data.volume_free_bytes >> 32) : private_data.volume_free_bytes);
            break;
        case SDCARD_COMMAND_GET_VOLUME_TOTAL:
            current_long_val = (uint32_t)((data == SDCARD_VOLUME_SPACE_DWORD_HIGH) ? (private_data.volume_total_bytes >> 32) : private_data.volume_total_bytes);
            break;
        case SDCARD_COMMAND_GET_DISK_SECTOR_COUNT:
            if (!sdcard_disk_refresh_sector_count())
            {
                current_long_val = 0;
            }
            else
            {
                current_long_val = (uint32_t)((data == SDCARD_DISK_SECTOR_DWORD_HIGH)
                    ? (private_data.disk_num_sectors >> 32)
                    : private_data.disk_num_sectors);
            }
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
                case SDCARD_COMMAND_REQUEST_FLASH_CHUNK:
                    sdcard_flash_chunk();
                    break;
                case SDCARD_COMMAND_REQUEST_REMOUNT:
                    sdcard_remount();
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_LIST:
                    sdcard_dir_list();
                    break;
                case SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_INDEX:
                    sdcard_file_open_from_index();
                    break;
                case SDCARD_COMMAND_REQUEST_OPEN_FILE_FROM_PATH:
                    sdcard_file_open_from_path();
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_INDEX:
                    sdcard_file_info_from_index();
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_INFO_FROM_PATH:
                    sdcard_file_info_from_path();
                    break;
                case SDCARD_COMMAND_REQUEST_VOLUME_SPACE:
                    sdcard_volume_space();
                    break;
                case SDCARD_COMMAND_REQUEST_DELETE_FROM_PATH:
                    sdcard_delete_from_path();
                    break;
                case SDCARD_COMMAND_REQUEST_CREATE_DIR_FROM_PATH:
                    sdcard_create_dir_from_path();
                    break;
                case SDCARD_COMMAND_REQUEST_CREATE_FILE_FROM_PATH:
                    sdcard_create_file_from_path();
                    break;
                case SDCARD_COMMAND_REQUEST_WRITE_FILE_CHUNK:
                    sdcard_file_write_chunk();
                    break;
                case SDCARD_COMMAND_REQUEST_DISK_READ_SECTOR:
                    sdcard_disk_read_sector();
                    break;
                case SDCARD_COMMAND_REQUEST_DISK_WRITE_SECTOR:
                    sdcard_disk_write_sector();
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
    private_data.cached_chunk_index = 0xffffffff;
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), SDCARD_QUEUE_BUFFER_LEN);
}