#include "legacy_display.h"

#include "../modxo_queue.h"
#include "../modxo_ports.h"

#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include <modxo_pinout.h>

#define LCD_QUEUE_BUFFER_LEN 1024
#define LCD_TIMEOUT_US 100

static uint8_t statusled = 1;

static struct
{
    MODXO_QUEUE_ITEM_T buffer[LCD_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;
    bool is_spi;
    uint8_t i2c_address;
} private_data;

void legacy_display_command(uint32_t cmd)
{
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.cmd = cmd;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

void legacy_display_data(uint8_t *data)
{
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = false;
    _item.data = *data;
    modxo_queue_insert(&private_data.queue, &_item);
    __sev();
}

void legacy_display_poll()
{
    MODXO_QUEUE_ITEM_T _item;
    if (modxo_queue_remove(&private_data.queue, &_item))
    {
        if (_item.iscmd)
        {
            MODXO_LCD_CMD rx_cmd;
            rx_cmd.raw = _item.cmd;

            switch (rx_cmd.cmd)
            {
            case MODXO_LCD_SET_SPI:
                legacy_display_set_spi();
                break;
            case MODXO_LCD_SET_I2C:
                legacy_display_set_i2c(rx_cmd.bytes[0]);
                break;
            default:
                break;
            }
        }
        else
        {
            if (private_data.is_spi)
            {
                gpio_put(LCD_PORT_SPI_CSN, 0);
                spi_write_blocking(LCD_PORT_SPI_INST, &_item.data, 1);
                gpio_put(LCD_PORT_SPI_CSN, 1);
                return;
            }
            i2c_write_timeout_us(LCD_PORT_I2C_INST, private_data.i2c_address, &_item.data, 1, false, LCD_TIMEOUT_US);
        }
    }
}

void legacy_display_set_spi()
{
    private_data.is_spi = true;
    spi_init(LCD_PORT_SPI_INST, 1 * 1000000);
    gpio_set_function(LCD_PORT_SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_PORT_SPI_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_PORT_SPI_CSN);
    gpio_put(LCD_PORT_SPI_CSN, 0);
    gpio_set_dir(LCD_PORT_SPI_CSN, GPIO_OUT);
}

void legacy_display_set_i2c(uint8_t i2c_address)
{
    private_data.is_spi = false;
    private_data.i2c_address = i2c_address;
    i2c_init(LCD_PORT_I2C_INST, 400 * 1000);
    gpio_set_function(LCD_PORT_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(LCD_PORT_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(LCD_PORT_I2C_SDA);
    gpio_pull_up(LCD_PORT_I2C_SCL);
}

void legacy_display_init()
{
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), LCD_QUEUE_BUFFER_LEN);
    legacy_display_set_spi();
}