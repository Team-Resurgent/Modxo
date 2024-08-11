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
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "pico/multicore.h"
#include "hardware/structs/bus_ctrl.h"

#include "../modxo_queue.h"
#include "../modxo_ports.h"

#include <modxo_pinout.h>
#include "hd44780.h"

/**
 *   I2C Bus
 */

#define LCD_CMD true
#define LCD_CHAR false

// commands
const int LCD_CLEARDISPLAY = 0x01;
const int LCD_RETURNHOME = 0x02;
const int LCD_ENTRYMODESET = 0x04;
const int LCD_DISPLAYCONTROL = 0x08;
const int LCD_CURSORSHIFT = 0x10;
const int LCD_FUNCTIONSET = 0x20;
const int LCD_SETCGRAMADDR = 0x40;
const int LCD_SETDDRAMADDR = 0x80;

const int LCD_I2C_TIMEOUT_US = 100;

#define BUFFER_LEN 1024

typedef struct
{
   uint8_t rows;
   uint8_t cols;
   bool use_en2;
   uint8_t row_addresses[4];
} LCD_PROPERTIES;

static struct
{
   uint8_t address;
   bool backlight;
   int selected_custom_char;
   bool increment_after_write;
   uint8_t crow;
   uint8_t ccol;
   uint8_t selected_custom_char_byte_count;
   MODXO_QUEUE_ITEM_T buffer[BUFFER_LEN];
   MODXO_QUEUE_T queue;
   LCD_PROPERTIES lcd_prop;
   uint8_t i2c_addr;

   union
   {
      struct
      {
         uint8_t rs : 1;
         uint8_t rw : 1;
         uint8_t E : 1;
         uint8_t bl_enable : 1;
         uint8_t nibble : 4;
      };
      uint8_t data;
   } i2c_lcd_port;

} private_data;

const LCD_PROPERTIES lcd_properties[] = {
    [HD44780_LCDTYPE_16X2] = {
        .rows = 2, .cols = 16, .row_addresses = {[0] = 0x00, [1] = 0x40}},

    [HD44780_LCDTYPE_16X4] = {.rows = 4, .cols = 16, .row_addresses = {
                                                         [0] = 0x00,
                                                         [1] = 0x40,
                                                         [2] = 0x10,
                                                         [3] = 0x50,
                                                     }},

    [HD44780_LCDTYPE_20X2] = {.rows = 2, .cols = 20, .row_addresses = {[0] = 0x00, [1] = 0x40}},

    [HD44780_LCDTYPE_20X4] = {.rows = 4, .cols = 20, .row_addresses = {[0] = 0x00, [1] = 0x40, [2] = 0x14, [3] = 0x54}},

    [HD44780_LCDTYPE_40X2] = {.rows = 2, .cols = 40, .row_addresses = {[0] = 0x00, [1] = 0x40}},

    [HD44780_LCDTYPE_40X4] = {.rows = 4, .cols = 40, .use_en2 = 1, .row_addresses = {[0] = 0x00, [1] = 0x40, [2] = 0x00, [3] = 0x40}},
};

static bool send_char(uint8_t data);

static bool set_lcd_pins()
{
   bool succesful = true;
   int retval = i2c_write_timeout_us(LCD_PORT_I2C_INST, private_data.address, &private_data.i2c_lcd_port.data, 1, false, LCD_I2C_TIMEOUT_US);

   if (retval == PICO_ERROR_TIMEOUT)
   {
      succesful = false;
      printf("\tI2C Timeout\n");
   }

   if (retval == PICO_ERROR_GENERIC)
   {
      succesful = false;
      printf("\tI2C GENERIC ERROR\n");
   }

   return succesful;
}

static bool send_nibble(uint8_t data, bool is_cmd)
{
   private_data.i2c_lcd_port.nibble = data;
   private_data.i2c_lcd_port.rs = is_cmd ? 0 : 1;
   private_data.i2c_lcd_port.bl_enable = private_data.backlight;

   private_data.i2c_lcd_port.E = 0;
   if (!set_lcd_pins())
      return false;
   sleep_ms(1);
   private_data.i2c_lcd_port.E = 1;
   if (!set_lcd_pins())
      return false;
   sleep_ms(1);
   private_data.i2c_lcd_port.E = 0;
   if (!set_lcd_pins())
      return false;
   sleep_ms(1);

   return true;
}

static bool send_byte(uint8_t data, bool is_cmd)
{
   if (!send_nibble(data >> 4, is_cmd))
      return false;
   if (!send_nibble(data & 0xF, is_cmd))
      return false;
   return true;
}

static bool set_backlight(bool on)
{
   private_data.backlight = on;
   private_data.i2c_lcd_port.bl_enable = private_data.backlight;
   private_data.i2c_lcd_port.E = 0;
   if (!set_lcd_pins())
      return false;
   return true;
}

/// HD44780 Commands ///

static bool clear_display(void)
{
   return send_byte(LCD_CLEARDISPLAY, LCD_CMD);
}

static bool return_home()
{
   return send_byte(LCD_RETURNHOME, LCD_CMD);
}

static bool entry_mode_set(bool inc_dec, bool shift_display)
{
   uint8_t cmd = LCD_ENTRYMODESET;
   private_data.increment_after_write = inc_dec;
   cmd |= inc_dec ? 2 : 0;
   cmd |= shift_display ? 1 : 0;
   return send_byte(cmd, LCD_CMD);
}

static bool display_control(bool display_ON, bool cursor_ON, bool blink_ON)
{
   uint8_t cmd = LCD_DISPLAYCONTROL;
   cmd |= display_ON ? 4 : 0;
   cmd |= cursor_ON ? 2 : 0;
   cmd |= blink_ON ? 1 : 0;
   return send_byte(cmd, LCD_CMD);
}

static bool cursor_display_shift(bool SC, bool RL)
{
   uint8_t cmd = LCD_DISPLAYCONTROL;
   cmd |= SC ? 8 : 0;
   cmd |= RL ? 4 : 0;
   return send_byte(cmd, LCD_CMD);
}

static bool function_set(bool DL, bool N, bool F)
{
   uint8_t cmd = LCD_FUNCTIONSET;
   cmd |= DL ? 16 : 0;
   cmd |= N ? 8 : 0;
   cmd |= F ? 4 : 0;
   return send_byte(cmd, LCD_CMD);
}

static bool set_cgram_address(uint8_t address)
{
   uint8_t cmd = LCD_SETCGRAMADDR;
   cmd |= address & 0x3F;
   return send_byte(cmd, LCD_CMD);
}

static bool set_ddram_address(uint8_t address)
{
   uint8_t cmd = LCD_SETDDRAMADDR;
   cmd |= address & 0x7F;

   return send_byte(cmd, LCD_CMD);
}

static bool set_cursor_position(uint8_t row, uint8_t col)
{
   private_data.crow = row;
   private_data.ccol = col;

   if (private_data.crow >= private_data.lcd_prop.rows || private_data.ccol >= private_data.lcd_prop.cols)
   {
      return false;
   }

   uint8_t address;
   address = private_data.lcd_prop.row_addresses[private_data.crow];
   address += private_data.ccol;
   return set_ddram_address(address);
}

// Initialization Command 4bit Interface
static bool init_lcd(void)
{
   if (!send_nibble(0x03, LCD_CMD))
      return false; // LCD_FUNCTIONSET | 0x1 8bit mode (This cmd is in 8bit mode)
   if (!send_nibble(0x03, LCD_CMD))
      return false; // LCD_FUNCTIONSET | 0x1 8bit mode (This cmd is in 8bit mode)
   if (!send_nibble(0x03, LCD_CMD))
      return false; // LCD_FUNCTIONSET | 0x1 8bit mode (This cmd is in 8bit mode)
   if (!send_nibble(0x02, LCD_CMD))
      return false; // LCD_FUNCTIONSET | 0x0 4bit mode (This cmd is in 8bit mode)
   sleep_ms(1);

   if (!function_set(false, true, false))
      return false; // 2 Line and 4bit mode
   if (!display_control(false, false, false))
      return false; // Display OFF
   if (!clear_display())
      return false;
   if (!entry_mode_set(true, false))
      return false; // Increment after sending char, shift display off
   if (!display_control(true, false, false))
      return false; // Display ON, Cursor OFF, Blink OFF
   if (!set_cursor_position(0, 0))
      return false;

   return true;
}

//////////////////////////////////////////////////////////////////////////

static bool edit_custom_character(uint8_t character_no)
{
   private_data.selected_custom_char = character_no;
   private_data.selected_custom_char_byte_count = 0;
   return set_cgram_address(character_no << 3);
}

static bool send_custom_char_data(uint8_t data)
{
   if (private_data.selected_custom_char != -1)
   {
      if (!send_byte(data, false))
         return false;
      private_data.selected_custom_char_byte_count++;
      if (private_data.selected_custom_char_byte_count == 8)
      {
         private_data.selected_custom_char = -1;
         if (!set_ddram_address(0))
            return false;
      }
   }

   return true;
}

static bool update_cursor_position(void)
{
   uint8_t address;
   address = private_data.lcd_prop.row_addresses[private_data.crow];
   address += private_data.ccol;
   return set_ddram_address(address);
}

static bool set_contrast_backlight_level(uint8_t reg_select, uint8_t backlight)
{
   if (reg_select == 1)
   {
      if (backlight)
      {
         private_data.backlight = true;
      }
      else
      {
         private_data.backlight = false;
      }
   }
   else
   {
      // Assuming 0-100 for contrast, backlight
      // If value is >= 0x80 ignore value, that way i can individually set contrast brightness
      //  TODO: Write PWM GPIO
   }

   return true;
}

static bool send_char(uint8_t data)
{
   if (private_data.crow >= private_data.lcd_prop.rows || private_data.ccol >= private_data.lcd_prop.cols)
   {
      return false;
   }

   bool retval = send_byte(data, LCD_CHAR);
   if (retval)
   {
      if (private_data.increment_after_write)
      {
         private_data.ccol++;
      }
      else
      {
         private_data.ccol--;
      }
   }

   return retval;
}

static bool write_char(uint8_t data)
{
   bool retval = true;

   if (private_data.selected_custom_char >= 0)
   {
      if (!set_cursor_position(private_data.crow, private_data.ccol))
      {
         return false;
      }
      private_data.selected_custom_char = -1;
   }

   switch (data)
   {
   case '\r':
      private_data.ccol = 0;
      break;
   case '\n':
      private_data.crow = (private_data.crow + 1) % private_data.lcd_prop.rows;
      break;
   case '\t':
      retval = send_char(' ') && retval;
      retval = send_char(' ') && retval;
      break;
   default:
      retval = send_char(data) && retval;
      break;
   }

   return retval;
}

static bool send_string(const char *s)
{
   while (*s)
   {
      if (!write_char(*s++))
         return false;
   }
}

static void poll()
{
   bool retval = true;
   MODXO_QUEUE_ITEM_T _item;
   if (modxo_queue_remove(&private_data.queue, &_item))
   {
      if (!_item.iscmd)
      {
         // Send Character
         write_char(_item.data);
      }
      else
      {
         // Parse Command
         MODXO_TD_CMD rx_cmd;
         rx_cmd.raw = _item.cmd;

         switch (rx_cmd.cmd)
         {
         case MODXO_TD_INIT:
            init_lcd();
            break;
         case MODXO_TD_CLEAR_DISPLAY:
            clear_display();
            set_cursor_position(0, 0);
            break;
         case MODXO_TD_RETURN_HOME:
            return_home();
            break;
         case MODXO_TD_ENTRY_MODE_SET:
            {
               bool increment = rx_cmd.param1 & 0x2;
               bool shift = rx_cmd.param1 & 0x1;
               entry_mode_set(increment, shift);
            }
            break;
         case MODXO_TD_DISPLAY_CONTROL:
            {
               bool display_ON = rx_cmd.param1 & 0x4;
               bool cursor_ON = rx_cmd.param1 & 0x2;
               bool blink_ON = rx_cmd.param1 & 0x1;
               display_control(display_ON, cursor_ON, blink_ON);
            }
            break;
         case MODXO_TD_CURSOR_DISPLAY_SHIFT:
            {
               bool SC = rx_cmd.param1 & 0x4;
               bool RL = rx_cmd.param1 & 0x2;
               cursor_display_shift(SC, RL);
            }
            break;
         case MODXO_TD_SET_CURSOR_POSITION:
            set_cursor_position(rx_cmd.param2, rx_cmd.param3);
            break;
         case MODXO_TD_SET_CONTRAST_BACKLIGHT:
            set_contrast_backlight_level(rx_cmd.param1, rx_cmd.param2);
            break;
         case MODXO_TD_SELECT_CUSTOM_CHAR:
            edit_custom_character(rx_cmd.param1);
            break;
         case MODXO_TD_SEND_CUSTOM_CHAR_DATA:
            send_custom_char_data(rx_cmd.param2);
            break;
         default:
            break;
         }
      }
   }
}

static void init_i2c_bus()
{
   i2c_init(LCD_PORT_I2C_INST, 400 * 1000);
   gpio_set_function(LCD_PORT_I2C_SDA, GPIO_FUNC_I2C);
   gpio_set_function(LCD_PORT_I2C_SCL, GPIO_FUNC_I2C);
   gpio_disable_pulls(LCD_PORT_I2C_SDA);
   gpio_disable_pulls(LCD_PORT_I2C_SCL);
}

void command(uint32_t cmd)
{
   MODXO_QUEUE_ITEM_T _item;
   _item.iscmd = true;
   _item.cmd = cmd;
   modxo_queue_insert(&private_data.queue, &_item);
   __sev();
}

void data(uint8_t *data)
{
   MODXO_QUEUE_ITEM_T _item;
   _item.iscmd = false;
   _item.data = *data;
   modxo_queue_insert(&private_data.queue, &_item);
   __sev();
}

int scan()
{
   int address = 0;
   uint8_t testvalue = 0;

   for (; address < 256; address++)
   {
      if (i2c_write_timeout_us(LCD_PORT_I2C_INST, address, &testvalue, 1, false, LCD_I2C_TIMEOUT_US) == 1)
      {
         break;
      }
   }

   if (address >= 256)
      return -1;
   else
      return address;
}

bool hd44780_i2c_init(MODXO_TD_DRIVER_T *drv, uint8_t address, HD44780_LCDTYPE lcdtype)
{
   bool retv = false;

   if (drv != NULL && lcdtype < HD44780_LCDTYPE_TOTAL_TYPES)
   {
      private_data.lcd_prop = lcd_properties[lcdtype];
      private_data.address = address;
      init_i2c_bus(LCD_PORT_I2C_INST, LCD_PORT_I2C_SDA, LCD_PORT_I2C_SCL);
      modxo_queue_init(&private_data.queue, (void *)private_data.buffer, sizeof(private_data.buffer[0]), BUFFER_LEN);
      drv->poll = poll;
      drv->data = data;
      drv->command = command;
      init_lcd();
      set_backlight(true);
      set_cursor_position(0, 0);
      send_string("Please Standby..."); // Hello World Test
      set_cursor_position(0, 0);
   }

   return retv;
}
