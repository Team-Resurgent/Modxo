/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"
#include "ws2812.h"
#include "../config/config_nvm.h"

#include "../lpc/lpc_interface.h"
#include "../lpc/lpc_regs.h"
#include "../modxo_ports.h"
#include "math.h"
#include <modxo_pinout.h>

#define WS2812_PORT_BASE MODXO_REGISTER_LED_COMMAND
#define WS2812_ADDRESS_MASK 0xFE
#define WS2812_COMMAND_PORT WS2812_PORT_BASE
#define WS2812_DATA_PORT WS2812_PORT_BASE + 1

#define LED_PIO pio1
#define IS_RGBW false // 24 bit color value

#define MAX_STRIPS 4
#define DISABLED_STRIP_VALUE 31

#define CMD_SET_LED_COUNT_STRIP 0x00
#define CMD_FILL_STRIP_COL 0x01
#define CMD_UPDATE_STRIPS 0x02
#define CMD_SET_PIX_IDX 0x03
#define CMD_WRITE_PIX_COL 0x04
#define CMD_SET_PIX_COUNT 0x05
#define CMD_WRITE_EFFECT 0x06
#define CMD_SELECT_COLOR 0xF0
#define CMD_SELECT_STRIP 0xFC

#define EFFECT_PIXELS_SHIFT_UP 0x01
#define EFFECT_PIXELS_SHIFT_DOWN 0x02
#define EFFECT_PIXELS_FADE 0x03
#define EFFECT_PIXELS_RANDOM 0x04
#define EFFECT_PIXELS_RAINBOW 0x05
#define EFFECT_PIXELS_BRIGHTNESS 0x06

typedef struct
{
    float red;
    float green;
    float blue;
} RGB_COLOR;

typedef struct
{
    RGB_COLOR rgb;
    float brightness;
} PIXEL_STATE;

typedef struct
{
    float h;
    float s;
    float v;
} HSV_COLOR;

typedef struct
{
    PIXEL_STATE pixels[256];
    uint next_led_to_display;
    uint total_leds;
    uint8_t selected_pixel;
    uint8_t gpio_pin;
} LED_STRIP;

uint8_t selected_strip;
bool updating_strips;

uint8_t ws2812_reg_buffer[4]; // Just in case a 32bit value register is added

uint32_t pixel_color_value;
REG24bit_t pixel_color_reg;

Effect_t pixel_effect_value;
REGEffect_t pixel_effect_reg;

uint8_t current_cmd;
uint8_t current_led_color = LedColorOff;

LED_STRIP strips[MAX_STRIPS] = {
    {
        .gpio_pin = LED_STRIP1,
    },
    {
        .gpio_pin = LED_STRIP2,
    },
    {
        .gpio_pin = LED_STRIP3,
    },
    {
        .gpio_pin = LED_STRIP4,
    }};

static RGB_COLOR hsv2rgb(HSV_COLOR hsv)
{
    int i = (int)(hsv.h / 60.0f) % 6;
    float f = (hsv.h / 60.0f) - i;
    float p = hsv.v * (1.0f - hsv.s);
    float q = hsv.v * (1.0f - f * hsv.s);
    float t = hsv.v * (1.0f - (1.0f - f) * hsv.s);

    RGB_COLOR rgb;

    switch (i)
    {
    case 0:
        rgb.red = hsv.v;
        rgb.green = t;
        rgb.blue = p;
        break;
    case 1:
        rgb.red = q;
        rgb.green = hsv.v;
        rgb.blue = p;
        break;
    case 2:
        rgb.red = p;
        rgb.green = hsv.v;
        rgb.blue = t;
        break;
    case 3:
        rgb.red = p;
        rgb.green = q;
        rgb.blue = hsv.v;
        break;
    case 4:
        rgb.red = t;
        rgb.green = p;
        rgb.blue = hsv.v;
        break;
    case 5:
        rgb.red = hsv.v;
        rgb.green = p;
        rgb.blue = q;
        break;
    }

    rgb.red *= 255.0f;
    rgb.green *= 255.0f;
    rgb.blue *= 255.0f;
    return rgb;
}

static HSV_COLOR rgb2hsv(RGB_COLOR rgb)
{
    float r = rgb.red / 255.0f;
    float g = rgb.green / 255.0f;
    float b = rgb.blue / 255.0f;

    float max = MAX(MAX(r, g), b);
    float min = MIN(MIN(r, g), b);
    float delta = max - min;

    HSV_COLOR hsv;
    hsv.v = max;

    if (max == 0)
    {
        hsv.s = 0;
    }
    else
    {
        hsv.s = delta / max;
    }

    if (delta == 0)
    {
        hsv.h = 0;
    }
    else
    {
        if (max == r)
        {
            hsv.h = 60 * (fmod(((g - b) / delta), 6.0f));
        }
        else if (max == g)
        {
            hsv.h = 60 * (((b - r) / delta) + 2.0f);
        }
        else if (max == b)
        {
            hsv.h = 60 * (((r - g) / delta) + 4.0f);
        }
    }

    if (hsv.h < 0)
    {
        hsv.h += 360;
    }

    return hsv;
}

static inline bool put_pixel(uint8_t strip, uint32_t pixel_color)
{
    if (pio_sm_is_tx_fifo_full(LED_PIO, strip))
        return true;

    pio_sm_put(LED_PIO, strip, pixel_color);
    return false;
}

static uint32_t traslate_pixel(PIXEL_STATE pixel, PIXEL_FORMAT_TYPE pixel_format)
{
    HSV_COLOR hsv = rgb2hsv(pixel.rgb);
    hsv.v *= (pixel.brightness / 255.0f);
    RGB_COLOR rgb = hsv2rgb(hsv);
    if (pixel_format == PIXEL_FORMAT_RGB)
    {
        return (((uint8_t)rgb.red) << 24) |
               (((uint8_t)rgb.green) << 16) | 
               (((uint8_t)rgb.blue) << 8); 
    }
    else if (pixel_format == PIXEL_FORMAT_GRB)
    {
        return (((uint8_t)rgb.green) << 24) |
               (((uint8_t)rgb.red) << 16) | 
               (((uint8_t)rgb.blue) << 8); 
    }
    else if (pixel_format == PIXEL_FORMAT_BRG)
    {
        return (((uint8_t)rgb.blue) << 24) |
               (((uint8_t)rgb.red) << 16) | 
               (((uint8_t)rgb.green) << 8); 
    }
    else if (pixel_format == PIXEL_FORMAT_RBG)
    {
        return (((uint8_t)rgb.red) << 24) |
               (((uint8_t)rgb.blue) << 16) | 
               (((uint8_t)rgb.green) << 8); 
    }
    else if (pixel_format == PIXEL_FORMAT_BGR)
    {
        return (((uint8_t)rgb.blue) << 24) |
               (((uint8_t)rgb.green) << 16) | 
               (((uint8_t)rgb.red) << 8); 
    }
    return (((uint8_t)rgb.green) << 24) |
           (((uint8_t)rgb.blue) << 16) | 
           (((uint8_t)rgb.red) << 8);
}

static RGB_COLOR traslate_rgb2color(uint32_t rgb_value)
{
    RGB_COLOR color;
    color.red = (float)(rgb_value & 0xff);
    color.green = (float)((rgb_value >> 8) & 0xff);
    color.blue = (float)((rgb_value >> 16) & 0xff);
    return color;
}

static uint32_t inline get_next_pixel_value(uint8_t strip)
{
    uint8_t display_led_no = strips[strip].next_led_to_display;
    PIXEL_FORMAT_TYPE pixel_format = (display_led_no == 0 && strip == 0) ? config.rgb_status_pf: config.rgb_strip_pf[strip];
    uint32_t display_color_value = traslate_pixel(strips[strip].pixels[display_led_no], pixel_format);
    return display_color_value;
}

static bool inline is_strip_update_inprogress(uint8_t strip)
{
    return strips[strip].next_led_to_display < strips[strip].total_leds &&
           strips[strip].gpio_pin < DISABLED_STRIP_VALUE;
}

static bool inline are_all_strips_updated()
{
    for (uint i = 0; i < 4; i++)
    {
        if (is_strip_update_inprogress(i) == true)
        {
            return false;
        }
    }
    return true;
}

static void set_pixel_count(uint8_t data)
{
    strips[selected_strip].total_leds = data + 1; // Total leds 1-256
}

static void enable_strip(uint8_t data)
{
}

static void fill_strip(uint32_t rgb24_color)
{
    RGB_COLOR color = traslate_rgb2color(rgb24_color);
    for (uint i = 0; i < strips[selected_strip].total_leds; i++)
    {
        strips[selected_strip].pixels[i].rgb = color;
    }
}

static void select_strip(uint8_t strip_no)
{
    if (strip_no < 4)
    {
        selected_strip = strip_no;
    }
}

static void select_pixel(uint8_t pixel_no)
{
    strips[selected_strip].selected_pixel = pixel_no;
}

static void write_pixel(uint32_t rgb24_color)
{
    strips[selected_strip].pixels[strips[selected_strip].selected_pixel++].rgb = traslate_rgb2color(rgb24_color);
}

static void write_pixel_effect(Effect_t effect)
{
    uint8_t startIndex = effect.endIndex > effect.startIndex ? effect.startIndex : effect.endIndex;
    uint8_t endIndex = effect.endIndex > effect.startIndex ? effect.endIndex : effect.startIndex;
    uint8_t count = (endIndex - startIndex) + 1;

    if (effect.type == EFFECT_PIXELS_SHIFT_UP)
    {
        PIXEL_STATE temp = strips[selected_strip].pixels[startIndex];
        while (startIndex < endIndex)
        {
            strips[selected_strip].pixels[startIndex] = strips[selected_strip].pixels[startIndex + 1];
            startIndex++;
        }
        strips[selected_strip].pixels[endIndex] = temp;
    }
    else if (effect.type == EFFECT_PIXELS_SHIFT_DOWN)
    {
        PIXEL_STATE temp = strips[selected_strip].pixels[endIndex];
        while (endIndex > startIndex)
        {
            strips[selected_strip].pixels[endIndex] = strips[selected_strip].pixels[endIndex - 1];
            endIndex--;
        }
        strips[selected_strip].pixels[startIndex] = temp;
    }
    else if (effect.type == EFFECT_PIXELS_FADE)
    {
        float fadeFactor = effect.param / 255.0f;
        for (uint8_t i = startIndex; i <= endIndex; i++)
        {
            HSV_COLOR hsv = rgb2hsv(strips[selected_strip].pixels[i].rgb);
            hsv.v *= fadeFactor;
            strips[selected_strip].pixels[i].rgb = hsv2rgb(hsv);
        }
    }
    else if (effect.type == EFFECT_PIXELS_RANDOM)
    {
        for (uint8_t i = startIndex; i <= endIndex; i++)
        {
            HSV_COLOR hsv;
            hsv.h = (float)(rand()) / RAND_MAX * 360.0f;
            hsv.s = 0.5f + (float)(rand()) / RAND_MAX * 0.5f;
            hsv.v = 0.5f + (float)(rand()) / RAND_MAX * 0.5f;
            strips[selected_strip].pixels[i].rgb = hsv2rgb(hsv);
        }
    }
    else if (effect.type == EFFECT_PIXELS_RAINBOW)
    {
        for (uint8_t i = startIndex; i <= endIndex; i++)
        {
            HSV_COLOR hsv;
            hsv.h = (float)(i - startIndex) / count * 360.0f;
            hsv.s = 1.0f;
            hsv.v = 1.0f;
            strips[selected_strip].pixels[i].rgb = hsv2rgb(hsv);
        }
    }
    else if (effect.type == EFFECT_PIXELS_BRIGHTNESS)
    {
        for (uint8_t i = startIndex; i <= endIndex; i++)
        {
            strips[selected_strip].pixels[i].brightness = effect.param;
        }
    }
}

static void lpc_port_read(uint16_t address, uint8_t *data)
{
    if (address == WS2812_COMMAND_PORT)
    {
        *data = are_all_strips_updated() ? 1 : 0;
    }
}

static void select_command(uint8_t cmd)
{
    // Clear regs
    LPC_REG_RESET(pixel_color_reg);
    LPC_REG_RESET(pixel_effect_reg);
    current_cmd = cmd;

    if (cmd == CMD_UPDATE_STRIPS)
    {
        ws2812_update_pixels();
    }
}

static void send_data(uint8_t data)
{
    switch (current_cmd)
    {
    case CMD_SET_LED_COUNT_STRIP:
        enable_strip(data);
        break;
    case CMD_FILL_STRIP_COL:
        LPC_REG_WRITE(pixel_color_reg, data);
        if (pixel_color_reg.reg.cwrite == 0)
        {
            fill_strip(pixel_color_value);
        }
        break;
    case CMD_SET_PIX_IDX:
        select_pixel(data);
        break;
    case CMD_WRITE_PIX_COL:
        LPC_REG_WRITE(pixel_color_reg, data);
        if (pixel_color_reg.reg.cwrite == 0)
        {
            write_pixel(pixel_color_value);
        }
        break;
    case CMD_WRITE_EFFECT:
        LPC_REG_WRITE(pixel_effect_reg, data);
        if (pixel_effect_reg.reg.cwrite == 0)
        {
            write_pixel_effect(pixel_effect_value);
        }
        break;
    case CMD_SELECT_STRIP:
        select_strip(data);
        break;

    case CMD_SET_PIX_COUNT:
        set_pixel_count(data);
        break;

    case CMD_SELECT_COLOR:
        ws2812_set_color(data);
        break;

    }
}

static void lpc_port_write(uint16_t address, uint8_t *data)
{
    switch (address)
    {
    case WS2812_COMMAND_PORT:
        select_command(*data);
        break;
    case WS2812_DATA_PORT:
        send_data(*data);
        break;
    }
}

// Update LEDs on main loop
void ws2812_poll()
{
    if (updating_strips)
    {
        for (uint display_strip = 0; display_strip < MAX_STRIPS; display_strip++)
        {
            while (is_strip_update_inprogress(display_strip))
            {
                bool fifo_full = put_pixel(display_strip, get_next_pixel_value(display_strip));

                if (fifo_full)
                    continue;

                strips[display_strip].next_led_to_display++;
            }
        }

        if (are_all_strips_updated())
        {
            updating_strips = false;
        }
    }
}

void ws2812_update_pixels()
{
    for (uint i = 0; i < 4; i++)
    {
        strips[i].next_led_to_display = 0;
    }
    updating_strips = true;
}

void ws2812_set_color(uint8_t color) {
    current_led_color = color;

    select_command(CMD_SELECT_STRIP);
    send_data(0);

    select_command(CMD_SET_PIX_COUNT);
    send_data(255);

    select_command(CMD_FILL_STRIP_COL);
    send_data((color & 1) == 1 ? 0xff : 0x00);
    send_data((color & 2) == 2 ? 0xff : 0x00);
    send_data((color & 4) == 4 ? 0xff : 0x00);

    select_command(CMD_UPDATE_STRIPS);
}

void ws2812_init()
{
    selected_strip = 0;
    updating_strips = false;

    LPC_REG_INIT(pixel_color_reg, pixel_color_value);
    LPC_REG_RESET(pixel_color_reg);

    LPC_REG_INIT(pixel_effect_reg, pixel_effect_value);
    LPC_REG_RESET(pixel_effect_reg);

    uint offset = pio_add_program(LED_PIO, &ws2812_program);

    for (uint i = 0; i < 4; i++)
    {
        for (uint j = 0; j < 256; j++)
        {
            strips[i].pixels[j].rgb.red = 0.0f;
            strips[i].pixels[j].rgb.green = 0.0f;
            strips[i].pixels[j].rgb.blue = 0.0f;
            strips[i].pixels[j].brightness = 255.0f;
        }
        strips[i].next_led_to_display = 0;
        strips[i].total_leds = 0;
        strips[i].selected_pixel = 0;
        if (strips[i].gpio_pin < DISABLED_STRIP_VALUE)
        {
            ws2812_program_init(LED_PIO, i, offset, strips[i].gpio_pin, 800000, IS_RGBW);
        }
    }

    lpc_interface_add_io_handler(WS2812_PORT_BASE, WS2812_ADDRESS_MASK, lpc_port_read, lpc_port_write);

    ws2812_update_pixels();
}