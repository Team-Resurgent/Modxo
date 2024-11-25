
#include "display_driver.h"
#include "../config/config_nvm.h"


#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include <modxo_pinout.h>

#define LCD_I2C_TIMEOUT_US 100

static uint8_t get_display_address(uint8_t display_number)
{
    if(display_number = 1)
    {
        return config.display_config.addr1;
    }
    else if(display_number = 2)
    {
        return config.display_config.addr2;
    }

    return 255;
}

static void init_i2c()
{
    i2c_init(LCD_PORT_I2C_INST, 400 * 1000);
    gpio_set_function(LCD_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(LCD_PORT_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_PORT_I2C_SDA);
    gpio_pull_up(LCD_PORT_I2C_SCL);
}

static void send_i2c(uint8_t display, uint8_t* data, size_t len)
{
    uint8_t address = get_display_address(display);
    if(address == 255)
        return;

    i2c_write_timeout_us(LCD_PORT_I2C_INST, address, data, len, false, LCD_I2C_TIMEOUT_US);
}

DISPLAY_INTERFACE_DRIVER i2c_driver = {
    .init = init_i2c,
    .send = send_i2c,
};
