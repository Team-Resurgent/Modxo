#include "display_interface.h"
#include "display_driver.h"

#include "../modxo_queue.h"
#include "../modxo_ports.h"

#include "../config/config_nvm.h"

#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include <modxo_pinout.h>

#define DISPLAY_INTERFACE_QUEUE_LEN 256

typedef struct
{
    uint8_t len;
    uint32_t data;
}DATA_ITEM_t;

static DATA_ITEM_t queue_buffer[DISPLAY_INTERFACE_QUEUE_LEN];
static MODXO_QUEUE_T queue;
static uint8_t selected_display; // 0 = none, 1 = display1,  2 = display2
static DISPLAY_INTERFACE current_interface;

static void interface_init(){
    selected_display = 1;

    switch(config.display_config.interface)
    {
        case DI_SPI:
            spi_driver.init();
        break;

        case DI_I2C:
            i2c_driver.init();
        break;
    }
}

static void send_data(uint8_t* data, uint16_t len){
    switch(config.display_config.interface)
    {
        case DI_SPI:
            spi_driver.send(selected_display, data, len);
        break;

        case DI_I2C:
            i2c_driver.send(selected_display, data, len);
        break;
    }
}

void display_select_display(uint8_t display_number)
{
    selected_display = display_number;
}

void display_init(void){
    modxo_queue_init(&queue, (void *)queue_buffer, sizeof(queue_buffer[0]), DISPLAY_INTERFACE_QUEUE_LEN);
    current_interface = config.display_config.interface;
    interface_init();
}

void display_task(void)
{
    DATA_ITEM_t item;

    if(current_interface != config.display_config.interface)
    {
        interface_init();
    }

    if (modxo_queue_remove(&queue, &item))
    {
        send_data((uint8_t*)(&item.data),item.len);
    }
}

void display_write(uint32_t data, uint8_t len)
{
    DATA_ITEM_t item;
    if(len >0 && len<5)//Only 1 - 4 bytes could be sent
    {
        item.data = data;
        item.len  = len;
        modxo_queue_insert(&queue, &item);
        data++;
        len--;
    }
}
