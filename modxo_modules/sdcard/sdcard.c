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
#include <sdcard.h>

#include <modxo.h>
#include <modxo/modxo_queue.h>
#include <modxo/modxo_ports.h>
#include <modxo/lpc_interface.h>

#include <pico.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"

#include <modxo_pinout.h>

#if SD_CARD_SPI_ENABLE
#include "ff.h"
#endif

#define MODXO_REGISTER_SDCARD_COMMAND 0xDEB0
#define MODXO_REGISTER_SDCARD_DATA 0xDEB1

#define SDCARD_QUEUE_BUFFER_LEN 1024

#define SDCARD_ROOT_MAX_ENTRIES 32
#define SDCARD_ROOT_NAME_MAX 255

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
    uint8_t id;
    uint8_t flags;
    char name[SDCARD_ROOT_NAME_MAX + 1];
} sdcard_root_entry_t;
#pragma pack(pop)

static uint8_t cmd_byte = SDCARD_COMMAND_NONE;

static struct
{
    MODXO_QUEUE_ITEM_T buffer[SDCARD_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;

    uint8_t root_list_ready;
    uint8_t root_list_count;
    uint8_t root_list_entry_sel;
    uint8_t root_name_byte_index;
    sdcard_root_entry_t root_entries[SDCARD_ROOT_MAX_ENTRIES];

    uint8_t file_op_ready;
    uint16_t file_chunk_length;
    uint8_t file_chunk_buffer[SDCARD_FILE_CHUNK_SIZE];
    uint32_t file_offset_staged;
    uint32_t file_size_cached;
    uint8_t file_read_result;
    uint8_t file_chunk_read_index;

#if SD_CARD_SPI_ENABLE
    FATFS fatfs;
    uint8_t sd_fat_mounted;
#endif
} private_data;

#if SD_CARD_SPI_ENABLE

static void ensure_mounted(void)
{
    if (private_data.sd_fat_mounted)
    {
        return;
    }
    if (f_mount(&private_data.fatfs, "0:", 1) == FR_OK)
    {
        private_data.sd_fat_mounted = 1;
    }
}

static void file_read_chunk_by_index(uint8_t index)
{
    private_data.file_chunk_length = 0;
    private_data.file_size_cached = 0;
    private_data.file_read_result = SDCARD_FILE_READ_RESULT_IDLE;

    if (index >= private_data.root_list_count)
    {
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_BAD_INDEX;
        return;
    }
    if (private_data.root_entries[index].flags & SDCARD_ROOT_FLAG_DIRECTORY)
    {
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_IS_DIRECTORY;
        return;
    }

    ensure_mounted();
    if (!private_data.sd_fat_mounted)
    {
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_ERROR;
        return;
    }

    char path[4 + SDCARD_ROOT_NAME_MAX + 1];
    snprintf(path, sizeof(path), "0:/%s", private_data.root_entries[index].name);

    FIL fp;
    FRESULT fr = f_open(&fp, path, FA_READ);
    if (fr != FR_OK)
    {
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_ERROR;
        return;
    }

    FSIZE_t fsz = f_size(&fp);
    if (fsz > (FSIZE_t)SDCARD_FILE_MAX_SIZE)
    {
        f_close(&fp);
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_TOO_LARGE;
        return;
    }

    uint32_t sz = (uint32_t)fsz;
    private_data.file_size_cached = sz;

    uint32_t off = private_data.file_offset_staged;
    if (off >= sz)
    {
        f_close(&fp);
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_OK;
        return;
    }

    uint32_t remain = sz - off;
    UINT to_read = (UINT)((remain > SDCARD_FILE_CHUNK_SIZE) ? SDCARD_FILE_CHUNK_SIZE : remain);

    fr = f_lseek(&fp, (FSIZE_t)off);
    if (fr != FR_OK)
    {
        f_close(&fp);
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_ERROR;
        private_data.file_size_cached = 0;
        return;
    }

    UINT br = 0;
    fr = f_read(&fp, private_data.file_chunk_buffer, to_read, &br);
    f_close(&fp);
    if (fr != FR_OK)
    {
        private_data.file_read_result = SDCARD_FILE_READ_RESULT_ERROR;
        private_data.file_size_cached = 0;
        private_data.file_chunk_length = 0;
        return;
    }

    private_data.file_chunk_length = (uint16_t)br;
    private_data.file_read_result = SDCARD_FILE_READ_RESULT_OK;
}

static void refresh_root_list(void)
{
    FRESULT fr;
    DIR dj;
    FILINFO fno;

    private_data.file_size_cached = 0;
    private_data.file_read_result = SDCARD_FILE_READ_RESULT_IDLE;
    private_data.file_chunk_length = 0;

    private_data.root_list_ready = 0;
    private_data.root_list_count = 0;

    if (!private_data.sd_fat_mounted)
    {
        fr = f_mount(&private_data.fatfs, "0:", 1);
        if (fr != FR_OK)
        {
            private_data.root_list_ready = 1;
            return;
        }
        private_data.sd_fat_mounted = 1;
    }

    memset(&dj, 0, sizeof(dj));
    memset(&fno, 0, sizeof(fno));
    fr = f_findfirst(&dj, &fno, "0:", "*");
    uint8_t n = 0;
    while (fr == FR_OK && fno.fname[0] != 0 && n < SDCARD_ROOT_MAX_ENTRIES)
    {
        if (fno.fname[0] == '.' && fno.fname[1] == '\0')
        {
            fr = f_findnext(&dj, &fno);
            continue;
        }
        if (fno.fname[0] == '.' && fno.fname[1] == '.' && fno.fname[2] == '\0')
        {
            fr = f_findnext(&dj, &fno);
            continue;
        }

        private_data.root_entries[n].id = n;
        private_data.root_entries[n].flags =
            (fno.fattrib & AM_DIR) ? (uint8_t)SDCARD_ROOT_FLAG_DIRECTORY : 0;
        strncpy(private_data.root_entries[n].name, fno.fname, SDCARD_ROOT_NAME_MAX);
        private_data.root_entries[n].name[SDCARD_ROOT_NAME_MAX] = '\0';
        n++;

        fr = f_findnext(&dj, &fno);
    }
    f_closedir(&dj);
    private_data.root_list_count = n;
    private_data.root_list_ready = 1;
}

#else /* !SD_CARD_SPI_ENABLE */

static void file_read_chunk_by_index(uint8_t index)
{
    (void)index;
    private_data.file_chunk_length = 0;
    private_data.file_size_cached = 0;
    private_data.file_read_result = SDCARD_FILE_READ_RESULT_ERROR;
}

static void refresh_root_list(void)
{
    private_data.root_list_count = 0;
    private_data.root_list_ready = 1;
}

#endif

static void SDCARD_poll()
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
                case SDCARD_COMMAND_REQUEST_ROOT_LIST_REFRESH:
                    refresh_root_list();
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK:
                    private_data.file_chunk_read_index = 0;
                    file_read_chunk_by_index(rx_cmd.param1);
                    private_data.file_op_ready = 1;
                    break;
                default:
                    break;
            }
        }
        __sev();
    }
}

static void queue_SDCARD_command(uint32_t cmd, uint32_t data)
{
    if (cmd == SDCARD_COMMAND_NONE)
    {
        return;
    }

    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.data.cmd = (uint8_t)cmd;
    _item.data.param1 = (uint8_t)data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

static void write_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
        case MODXO_REGISTER_SDCARD_DATA:
        {
            switch (cmd_byte)
            {
                case SDCARD_COMMAND_SET_ROOT_LIST_ENTRY_INDEX:
                    private_data.root_list_entry_sel = *data;
                    break;
                case SDCARD_COMMAND_SET_ROOT_LIST_NAME_INDEX:
                    private_data.root_name_byte_index = *data;
                    break;
                case SDCARD_COMMAND_SET_FILE_OFFSET_B0:
                    private_data.file_offset_staged =
                        (private_data.file_offset_staged & ~(0xffu << 0)) |
                        ((uint32_t)(*data & 0xffu) << 0);
                    break;
                case SDCARD_COMMAND_SET_FILE_OFFSET_B1:
                    private_data.file_offset_staged =
                        (private_data.file_offset_staged & ~(0xffu << 8)) |
                        ((uint32_t)(*data & 0xffu) << 8);
                    break;
                case SDCARD_COMMAND_SET_FILE_OFFSET_B2:
                    private_data.file_offset_staged =
                        (private_data.file_offset_staged & ~(0xffu << 16)) |
                        ((uint32_t)(*data & 0xffu) << 16);
                    break;
                case SDCARD_COMMAND_SET_FILE_OFFSET_B3:
                    private_data.file_offset_staged =
                        (private_data.file_offset_staged & ~(0xffu << 24)) |
                        ((uint32_t)(*data & 0xffu) << 24);
                    break;
                case SDCARD_COMMAND_SET_FILE_CHUNK_READ_INDEX:
                    private_data.file_chunk_read_index = *data;
                    break;
            }
            break;
        }
        case MODXO_REGISTER_SDCARD_COMMAND:
        {
            switch (*data)
            {
                case SDCARD_COMMAND_REQUEST_ROOT_LIST_REFRESH:
                    private_data.root_list_ready = 0;
                    cmd_byte = *data;
                    queue_SDCARD_command(*data, 0);
                    break;
                case SDCARD_COMMAND_REQUEST_FILE_READ_CHUNK:
                    cmd_byte = *data;
                    private_data.file_op_ready = 0;
                    queue_SDCARD_command(*data, private_data.root_list_entry_sel);
                    break;
                case SDCARD_COMMAND_SET_ROOT_LIST_ENTRY_INDEX:
                case SDCARD_COMMAND_SET_ROOT_LIST_NAME_INDEX:
                case SDCARD_COMMAND_SET_FILE_OFFSET_B0:
                case SDCARD_COMMAND_SET_FILE_OFFSET_B1:
                case SDCARD_COMMAND_SET_FILE_OFFSET_B2:
                case SDCARD_COMMAND_SET_FILE_OFFSET_B3:
                case SDCARD_COMMAND_SET_FILE_CHUNK_READ_INDEX:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_READY:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_COUNT:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_ID:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_FLAGS:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_LENGTH:
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_BYTE:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_OP_READY:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_LO:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_HI:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_BYTE:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B0:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B1:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B2:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B3:
                case SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT:
                    cmd_byte = *data;
                    break;
                default:
                    cmd_byte = SDCARD_COMMAND_NONE;
            }
            break;
        }
        break;
    }
}

static void read_handler(uint16_t address, uint8_t *data)
{
    switch (address)
    {
        case MODXO_REGISTER_SDCARD_DATA:
        {
            switch (cmd_byte)
            {
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_READY:
                    *data = private_data.root_list_ready;
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_COUNT:
                    *data = private_data.root_list_count;
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_ID:
                    if (private_data.root_list_entry_sel < private_data.root_list_count)
                    {
                        *data = private_data.root_entries[private_data.root_list_entry_sel].id;
                    }
                    else
                    {
                        *data = 0;
                    }
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_ENTRY_FLAGS:
                    if (private_data.root_list_entry_sel < private_data.root_list_count)
                    {
                        *data = private_data.root_entries[private_data.root_list_entry_sel].flags;
                    }
                    else
                    {
                        *data = 0;
                    }
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_LENGTH:
                    if (private_data.root_list_entry_sel < private_data.root_list_count)
                    {
                        *data = (uint8_t)strlen(
                            private_data.root_entries[private_data.root_list_entry_sel].name);
                    }
                    else
                    {
                        *data = 0;
                    }
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_ROOT_LIST_NAME_BYTE:
                    if (private_data.root_list_entry_sel < private_data.root_list_count)
                    {
                        const char *nm =
                            private_data.root_entries[private_data.root_list_entry_sel].name;
                        uint8_t len = (uint8_t)strlen(nm);
                        if (private_data.root_name_byte_index < len)
                        {
                            *data = (uint8_t)nm[private_data.root_name_byte_index];
                            private_data.root_name_byte_index++;
                        }
                        else
                        {
                            *data = 0;
                        }
                    }
                    else
                    {
                        *data = 0xff;
                    }
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_OP_READY:
                    *data = private_data.file_op_ready;
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_LO:
                    *data = (uint8_t)((private_data.file_chunk_length >> 0) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_LENGTH_HI:
                    *data = (uint8_t)((private_data.file_chunk_length >> 8) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_CHUNK_BYTE:
                    if (private_data.file_chunk_read_index < private_data.file_chunk_length)
                    {
                        *data = private_data.file_chunk_buffer[private_data.file_chunk_read_index];
                        private_data.file_chunk_read_index++;
                    }
                    else
                    {
                        *data = 0;
                    }
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B0:
                    *data = (uint8_t)((private_data.file_size_cached >> 0) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B1:
                    *data = (uint8_t)((private_data.file_size_cached >> 8) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B2:
                    *data = (uint8_t)((private_data.file_size_cached >> 16) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_SIZE_B3:
                    *data = (uint8_t)((private_data.file_size_cached >> 24) & 0xffu);
                    break;
                case SDCARD_COMMAND_GET_RESPONSE_FILE_READ_RESULT:
                    *data = private_data.file_read_result;
                    break;
                default:
                    *data = 0xff;
                    break;
            }
            break;
        }
    }
}

static void powerup(void)
{
    cmd_byte = SDCARD_COMMAND_NONE;
}

static void init()
{
    memset(&private_data, 0, sizeof(private_data));
    private_data.file_op_ready = 1;
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), SDCARD_QUEUE_BUFFER_LEN);

    lpc_interface_add_io_handler(MODXO_REGISTER_SDCARD_COMMAND, 0xFFFE, read_handler, write_handler);
}


MODXO_TASK sdcard_hdlr = {
    .init = init,
    .powerup = powerup,
    .core0_poll = SDCARD_poll
};
