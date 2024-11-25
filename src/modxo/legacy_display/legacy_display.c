#include "legacy_display.h"
#include "../config/config_nvm.h"
#include "../display_interface/display_interface.h"

#include "../modxo_queue.h"
#include "../modxo_ports.h"

#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include <modxo_pinout.h>

#define LCD_QUEUE_BUFFER_LEN 1024
#define LCD_TIMEOUT_US 100

static struct
{
    MODXO_QUEUE_ITEM_T buffer[LCD_QUEUE_BUFFER_LEN];
    MODXO_QUEUE_T queue;
    bool is_spi;
    uint8_t i2c_address;
    bool has_i2c_prefix;
    uint8_t i2c_prefix;
} private_data;

void legacy_display_command(uint32_t raw)
{
    MODXO_QUEUE_ITEM_T _item;
    _item.iscmd = true;
    _item.raw = raw;
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
            rx_cmd.raw = _item.raw;

            switch (rx_cmd.cmd)
            {
            case MODXO_LCD_SET_SPI:
                legacy_display_set_spi();
                break;
            case MODXO_LCD_SET_I2C:
                legacy_display_set_i2c(rx_cmd.param1);
                break;
            case MODXO_LCD_REMOVE_I2C_PREFIX:
                legacy_display_remove_i2c_prefix();
                break;
            case MODXO_LCD_SET_I2C_PREFIX:
                legacy_display_set_i2c_prefix(rx_cmd.param1);
                break;
            default:
                break;
            }
        }
        else
        {
            if (private_data.has_i2c_prefix)
            {
                char tempBuffer[4];
                tempBuffer[0] = private_data.i2c_prefix;
                tempBuffer[1] = _item.data;
                display_write(*((uint32_t*)tempBuffer),2);
            }
            else
            {
                display_write(_item.data,1);
            }
        }
    }
}

void legacy_display_set_spi()
{
    legacy_display_remove_i2c_prefix();
    display_set_interface(DI_SPI);
}

void legacy_display_set_i2c(uint8_t i2c_address)
{
    config.display_config.addr1 = i2c_address;
    display_set_interface(DI_I2C);
}

void legacy_display_remove_i2c_prefix()
{
    private_data.has_i2c_prefix = false;
}

void legacy_display_set_i2c_prefix(uint8_t prefix)
{
    private_data.has_i2c_prefix = true;
    private_data.i2c_prefix = prefix;
}

void legacy_display_init()
{
    modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), LCD_QUEUE_BUFFER_LEN);
    display_set_interface(config.display_config.interface);
}
