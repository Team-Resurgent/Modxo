/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

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

#include <stdio.h>
#include <pico.h>
#include <hardware/sync.h>
#include <hardware/flash.h>
#include <hardware/regs/addressmap.h>
#include <hardware/sync.h>

#include <modxo/lpc_interface.h>
#include <modxo/config_nvm.h>
#include <string.h>

#define NVM_FLASH_OFFSET 0x3D000
#define NVM_FLASH_SECTORS 2
#define NVM_FLASH_SECTOR_SIZE 4096


#define NVM_PAGE_SIZE 256
#define NVM_TOTAL_PAGES ( (NVM_FLASH_SECTORS*NVM_FLASH_SECTOR_SIZE) / NVM_PAGE_SIZE )

typedef struct
{
    uint8_t data[NVM_PAGE_SIZE - 2];
    uint16_t crc;
} NVM_PAGE;

static bool save_config=false;
static NVM_PAGE* nvm_pages= (NVM_PAGE*)(NVM_FLASH_OFFSET + XIP_BASE);


static uint16_t crc_ccitt_false[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};




static uint16_t calc_crc(const void* data, uint16_t len) {
    uint16_t calc = 0xFFFF;
    const uint8_t* buf = (const uint8_t*)data;
    
    for (;len; len--) {
        calc = (calc << 8) ^ crc_ccitt_false[ ((calc >> 8) ^ (*buf++)) & 0xFF];
    }

    return calc;
}


static bool is_page_empty(uint8_t page_no){
    bool is_empty = true;
    uint32_t* flash_ptr = (uint32_t*)&nvm_pages[page_no];

    for(uint i =0;i<(NVM_PAGE_SIZE/4);i++){
        if(*flash_ptr != ((uint32_t)0xFFFFFFFF)){
            is_empty = false;
            break;
        }
        flash_ptr++;
    }

    return is_empty;    
}

static bool is_sector_empty(uint8_t sectorno){
    bool is_empty = true;
    uint32_t* flash_ptr = (uint32_t*)(NVM_FLASH_OFFSET + sectorno*NVM_FLASH_SECTOR_SIZE+XIP_BASE);

    for(uint i =0;i<(NVM_FLASH_SECTOR_SIZE/4);i++){
        if(*flash_ptr != 0xFFFFFFFF){
            is_empty = false;
            break;
        }
    }

    return is_empty;
}



static bool is_page_valid(uint8_t page_no){
    bool is_valid = false;
    uint16_t crc;
    NVM_PAGE* page = &nvm_pages[page_no];

    if(calc_crc((uint8_t*)page, NVM_PAGE_SIZE) == 0){
        is_valid = true;
    }

    return is_valid;
}

static int look_next_empty_page(int page_no){
    int empty_page_no=-1;
    for(int i = page_no+1; i< NVM_TOTAL_PAGES;i++){
        if(is_page_empty(i)){
            empty_page_no = i;
            break;
        }
    }

    return empty_page_no;
}

static int look_last_config(){
    int page = -1;
    for(page = NVM_TOTAL_PAGES-1; page >= 0; page--){
        if(is_page_valid(page)){
            break;
        }
    }

    return page;
}


static void erase_nvm_sector(uint8_t sectorno){
    uint32_t flash_offset = NVM_FLASH_OFFSET + sectorno*NVM_FLASH_SECTOR_SIZE;
    flash_range_erase(flash_offset, NVM_FLASH_SECTOR_SIZE);
}

static void program_nvm_page(uint8_t number, NVM_PAGE* buffer){
    uint32_t flash_offset = NVM_FLASH_OFFSET + number * NVM_PAGE_SIZE;
    flash_range_program(flash_offset, (void*)buffer, NVM_PAGE_SIZE);
}

static void prepare_nvm_page(NVM_PAGE* page)
{
    uint8_t *dst = page->data;
    memset(dst, 0xFF, sizeof(NVM_PAGE));

    for(int i = 1; i < nvm_registers_count; i++){
        memcpy(dst, nvm_registers[i]->data, nvm_registers[i]->size);
        dst += nvm_registers[i]->size;
    }
    
    uint16_t crc = calc_crc((uint8_t*)page,sizeof(NVM_PAGE)-2);
    page->crc = (crc<<8) | (crc>>8); //CRC must be stored as big endian
}

static void load_nvm_defaults()
{
    for(int i = 1; i < nvm_registers_count; i++){
        memcpy(nvm_registers[i]->data, nvm_registers[i]->default_value, nvm_registers[i]->size);
    }
}

static void load_nvm_values(int page_no){
    NVM_PAGE* page = &nvm_pages[page_no];
    uint8_t *src = page->data;
    
    for(int i = 1; i < nvm_registers_count; i++){
        memcpy(nvm_registers[i]->data, src, nvm_registers[i]->size);
        src += nvm_registers[i]->size;
    }
}

static void nvm_save_page(int page_no){
    NVM_PAGE page;
    uint32_t ints;
    
    if(page_no == -1){
        page_no = 0;
        
        prepare_nvm_page(nvm_pages);

        ints = save_and_disable_interrupts();
        if(!is_sector_empty(0)){
            erase_nvm_sector(0);
        }
        
        if(!is_sector_empty(1)){
            erase_nvm_sector(1);
        }

        program_nvm_page(page_no,&page);
        restore_interrupts (ints);
    } else {
        prepare_nvm_page(&nvm_pages[page_no]);
        ints = save_and_disable_interrupts();
        program_nvm_page(page_no, &page);
        restore_interrupts (ints);
    }

}

void config_save_parameters(){
    save_config=true;
    __sev();
}

void config_poll()
{
    if(save_config)
    {
        int page_no;
        //Save it after the last valid config
        page_no = look_last_config();
        page_no = look_next_empty_page(page_no);

        nvm_save_page(page_no);
        save_config=false;
    }
}

void config_retrieve_parameters(){
    int page_no = look_last_config();
    if(page_no == -1)
    {
        
        if(is_sector_empty(0)){
           page_no = 0; 
        }
        
        load_nvm_defaults();
        nvm_save_page(page_no);
    }
    else
    {
        load_nvm_values(page_no);
    }
}

