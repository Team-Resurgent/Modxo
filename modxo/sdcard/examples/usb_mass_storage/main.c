/*
 * Driver for Raspberry Pi Pico SD card block device
 */
/*
 * Modifications copyright 2024 Carl John Kugler III
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may not use
 * this file except in compliance with the License. You may obtain a copy of the
 * License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 */
/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <bsp/board.h>
#include <pico/stdlib.h>
#include <stdlib.h>
#include <string.h>
#include <tusb.h>

/**
 * @file main.c
 * @brief Entry point for the Raspberry Pi Pico TinyUSB CDC ACM and MSC
 *       example.
 *
 * This example implements a CDC ACM device and a Mass Storage device.
 * The CDC ACM device enumerates as a serial port and can be used to
 * transfer data to and from the board. The Mass Storage device enumerates
 * as a USB drive and can be used to transfer files to and from the board.
 *
 * The example uses the TinyUSB library for USB communication and the
 * Raspberry Pi Pico SDK for hardware access.
 */
int main(void) {
    // Initialize the board
    board_init();

    // Clear Screen
    printf("\033[2J\033[H");

    // Initialize TinyUSB
    tud_init(BOARD_TUD_RHPORT);

    // Initialize the standard I/O streams
    stdio_init_all();

    // Run the TinyUSB task loop
    while (true) {
        tud_task();
    }
}
