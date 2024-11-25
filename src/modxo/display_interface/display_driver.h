#pragma once
#include <stdint.h>
#include <stddef.h>

typedef struct
{
    void (*init)();
    void (*send)(uint8_t display_number, uint8_t* data, size_t len);
}DISPLAY_INTERFACE_DRIVER;


extern DISPLAY_INTERFACE_DRIVER i2c_driver, spi_driver;