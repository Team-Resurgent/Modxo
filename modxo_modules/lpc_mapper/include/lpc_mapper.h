#pragma once

#include <modxo.h>
#include <stdint.h>

#define LPC_MAPPER_VERSION                           0x01
#define NUM_LPC_MAPPERS                              4
#define LPC_MAPPER_IO_BASE                           0x2888
#define LPC_MAPPER_LONG_DATA_PORT_BASE               (LPC_MAPPER_IO_BASE + 0)
#define LPC_MAPPER_CMD_PORT                          (LPC_MAPPER_IO_BASE + 4)
#define LPC_MAPPER_DATA_PORT                         (LPC_MAPPER_IO_BASE + 5)
#define LPC_MAPPER_VERSION_PORT                      (LPC_MAPPER_IO_BASE + 7)

#define LPC_MAPPER_CMD_ENABLE                        0
#define LPC_MAPPER_CMD_CUR_MAPPER_ID                 1
#define LPC_MAPPER_CMD_MAPPER_BASE_ADDR              2
#define LPC_MAPPER_CMD_MAPPER_LENGTH                 3
#define LPC_MAPPER_CMD_MAPPER_TYPE                   4
#define LPC_MAPPER_CMD_MAPPER_FLAGS                  5
#define LPC_MAPPER_CMD_MAPPER_CTRL                   6
#define LPC_MAPPER_CMD_MAPPER_CTRL_CMD               7
#define LPC_MAPPER_CMD_MAPPER_CTRL_DATA              8
#define LPC_MAPPER_CMD_SHORTCUT_ENABLE               9

#define LPC_MAPPER_TYPE_NONE                         0
#define LPC_MAPPER_TYPE_PSRAM                        1
#define LPC_MAPPER_TYPE_INTERNAL_RAM                 2
#define LPC_MAPPER_TYPE_RNG                          3
#define LPC_MAPPER_TYPE_ECHO                         4
#define LPC_MAPPER_TYPE_HEAP                         5
#define LPC_MAPPER_TYPE_SDCARD                       6
#define LPC_MAPPER_TYPE_EXPANSION                    7
#define LPC_MAPPER_TYPE_FLASH                        8

#define LPC_MAPPER_FLAG_WRITE_ENABLED                (1 << 0)
#define LPC_MAPPER_FLAG_READ_ENABLED                 (1 << 1)


typedef struct {
	uint32_t base_addr;
	uint32_t length;
	uint8_t flags;
	bool (*read)(uint32_t addr, uint8_t *data, uint8_t mapper_id);
	bool (*write)(uint32_t addr, uint8_t *data, uint8_t mapper_id);
	uint8_t (*control)(uint8_t cmd, uint8_t data, bool is_read);
} LPC_MAPPER;

extern LPC_MAPPER lpc_mappers[];
extern MODXO_TASK lpc_mapper_hdlr;
extern uint8_t current_lpc_mapper_cmd;
extern uint32_t current_long_val;
extern uint8_t current_mapper_id;
extern uint8_t current_mapper_hdlr_cmd;
extern uint8_t current_mapper_hdlr_data;
extern bool lpc_mapper_enabled;

extern bool shortcut_enabled;
extern uint8_t shortcut_mapper_id;
extern uint32_t shortcut_base_addr;
extern uint32_t shortcut_buffer_size;
extern uint8_t *shortcut_buffer;


bool lpc_mapper_custom_read_handler(uint32_t addr, uint8_t *data);
bool lpc_mapper_custom_write_handler(uint32_t addr, uint8_t *data);

bool psram_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool psram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t psram_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void psram_handler_init();

bool int_ram_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool int_ram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t int_ram_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void int_ram_handler_powerup();

bool heap_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool heap_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t heap_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void heap_handler_powerup();

bool rng_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool rng_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t rng_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void rng_handler_powerup();

bool echo_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool echo_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t echo_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void echo_handler_powerup();

bool sdcard_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool sdcard_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t sdcard_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void sdcard_handler_poll();
void sdcard_handler_powerup();
void sdcard_handler_init();

bool expansion_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool expansion_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t expansion_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void expansion_handler_poll();
void expansion_handler_powerup();
void expansion_handler_init();

bool flash_memread_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
bool flash_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t mapper_id);
uint8_t flash_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void flash_handler_powerup();
