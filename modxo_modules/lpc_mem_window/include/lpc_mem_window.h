#pragma once

#include <modxo.h>
#include <stdint.h>

#define LPC_MEM_WIN_VERSION                           0x01
#define NUM_LPC_MEM_WINDOWS                           4
#define LPC_MEM_WIN_IO_BASE                           0x2888
#define LPC_MEM_WIN_LONG_DATA_PORT_BASE               (LPC_MEM_WIN_IO_BASE + 0)
#define LPC_MEM_WIN_CMD_PORT                          (LPC_MEM_WIN_IO_BASE + 4)
#define LPC_MEM_WIN_DATA_PORT                         (LPC_MEM_WIN_IO_BASE + 5)
#define LPC_MEM_WIN_VERSION_PORT                      (LPC_MEM_WIN_IO_BASE + 7)

#define LPC_MEM_WIN_CMD_ENABLE                        0
#define LPC_MEM_WIN_CMD_CUR_WIN_ID                    1
#define LPC_MEM_WIN_CMD_WIN_BASE_ADDR                 2
#define LPC_MEM_WIN_CMD_WIN_LENGTH                    3
#define LPC_MEM_WIN_CMD_WIN_TYPE                      4
#define LPC_MEM_WIN_CMD_WIN_FLAGS                     5
#define LPC_MEM_WIN_CMD_WIN_CTRL                      6
#define LPC_MEM_WIN_CMD_WIN_CTRL_CMD                  7

#define LPC_MEM_WIN_TYPE_NONE                         0
#define LPC_MEM_WIN_TYPE_PSRAM                        1
#define LPC_MEM_WIN_TYPE_INTERNAL_RAM                 2
#define LPC_MEM_WIN_TYPE_RNG                          3
#define LPC_MEM_WIN_TYPE_ECHO                         4
#define LPC_MEM_WIN_TYPE_SDCARD                       5

#define LPC_MEM_WIN_FLAG_WRITE_ENABLED                (1 << 0)
#define LPC_MEM_WIN_FLAG_READ_ENABLED                 (1 << 1)


typedef struct {
	uint32_t base_addr;
	uint32_t length;
	uint8_t flags;
	bool (*read)(uint32_t addr, uint8_t *data, uint8_t window_id);
	bool (*write)(uint32_t addr, uint8_t *data, uint8_t window_id);
	uint8_t (*control)(uint8_t cmd, uint8_t data, bool is_read);
} LPC_MEM_WINDOW;

extern LPC_MEM_WINDOW lpc_mem_windows[];
extern MODXO_TASK lpc_mem_window_hdlr;
extern uint8_t current_lpc_mem_win_cmd;
extern uint32_t current_long_val;
extern uint8_t current_window_id;
extern uint8_t current_win_hdlr_cmd;
extern uint8_t lpc_mem_window_enabled;


bool lpc_mem_window_custom_read_handler(uint32_t addr, uint8_t *data);
bool lpc_mem_window_custom_write_handler(uint32_t addr, uint8_t *data);

bool psram_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
bool psram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
uint8_t psram_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void psram_handler_init();

bool int_ram_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
bool int_ram_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
uint8_t int_ram_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void int_ram_handler_powerup();

bool rng_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
bool rng_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
uint8_t rng_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void rng_handler_powerup();

bool echo_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
bool echo_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
uint8_t echo_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void echo_handler_powerup();

bool sdcard_memread_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
bool sdcard_memwrite_handler(uint32_t addr, uint8_t *data, uint8_t window_id);
uint8_t sdcard_handler_control(uint8_t cmd, uint8_t data, bool is_read);
void sdcard_handler_poll();
void sdcard_handler_powerup();
void sdcard_handler_init();