
#include "display_driver.h"


#include <pico.h>
#include <pico/stdlib.h>
#include "pico/multicore.h"
#include "pico/binary_info.h"

#include <modxo_pinout.h>

static void select_display(uint8_t displayn)
{
            switch(displayn)
            {
                case 0://No display selected
                    gpio_put(LCD_PORT_SPI_CSN, 1);
                    gpio_put(LCD_PORT_SPI_CSN2, 1);
                break;

                case 1:
                    gpio_put(LCD_PORT_SPI_CSN, 0);
                    gpio_put(LCD_PORT_SPI_CSN2, 1);
                break;

                case 2:
                    gpio_put(LCD_PORT_SPI_CSN, 1);
                    gpio_put(LCD_PORT_SPI_CSN2, 0);
                break;
            }
}

static void init_spi()
{
    spi_init(LCD_PORT_SPI_INST, 1 * 1000000);
    gpio_set_function(LCD_PORT_SPI_CLK, GPIO_FUNC_SPI);
    gpio_set_function(LCD_PORT_SPI_MOSI, GPIO_FUNC_SPI);

    gpio_init(LCD_PORT_SPI_CSN);
    gpio_init(LCD_PORT_SPI_CSN2);
    gpio_set_dir(LCD_PORT_SPI_CSN, GPIO_OUT);
    gpio_set_dir(LCD_PORT_SPI_CSN2, GPIO_OUT);

    select_display(1);
}

static void send_spi(uint8_t display, uint8_t* data, size_t len)
{
    select_display(display);
    spi_write_blocking(LCD_PORT_SPI_INST, data, len);
    select_display(0);//Both CSN = 1
}

DISPLAY_INTERFACE_DRIVER spi_driver = {
    .init = init_spi,
    .send = send_spi
};
