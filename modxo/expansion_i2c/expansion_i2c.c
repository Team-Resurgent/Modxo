#include <expansion_i2c.h>

#include <hardware/gpio.h>
#include <hardware/i2c.h>
#include <modxo.h>
#include <modxo_pinout.h>

static bool expansion_initialized = false;

void expansion_i2c_init()
{
    if (expansion_initialized) {
        return;
    }
    expansion_initialized = true;
    i2c_init(EXPANSION_PORT_I2C_INST, 400 * 1000);
    gpio_set_function(EXPANSION_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(EXPANSION_PORT_I2C_SCL, GPIO_FUNC_I2C);
}

int expansion_i2c_read_timeout_us(uint8_t addr, uint8_t *dst, size_t len, bool nostop, uint timeout_us) {
    expansion_i2c_init();
    return i2c_read_timeout_us(EXPANSION_PORT_I2C_INST, addr, dst, len, nostop, timeout_us);
}

int expansion_i2c_write_timeout_us(uint8_t addr, uint8_t *src, size_t len, bool nostop, uint timeout_us) {
    expansion_i2c_init();
    return i2c_write_timeout_us(EXPANSION_PORT_I2C_INST, addr, src, len, nostop, timeout_us);
}