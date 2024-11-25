#pragma once

#include <stdint.h>

typedef enum {
  DI_SPI = 0,
  DI_I2C = 1,
}DISPLAY_INTERFACE;

void display_init(void);
void display_select_display(uint8_t display_number);
void display_write(uint32_t data, uint8_t bytes_to_send);
void display_task(void);
void display_set_interface(DISPLAY_INTERFACE interface);
