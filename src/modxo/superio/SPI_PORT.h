/*
SPDX short identifier: BSD-2-Clause
BSD 2-Clause License

Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
*/
#pragma once
#include <hardware/spi.h>
#define SPI_DISPLAY_INSTANCE spi1
#define SPI_DISPLAY_BAUDRATE 500 * 1000
#define SPI_DISPLAY_RX_PIN 12
#define SPI_DISPLAY_SCK_PIN 14
#define SPI_DISPLAY_TX_PIN 15
#define SPI_DISPLAY_SCN_PIN 13

void spi_port_init(spi_inst_t *spi, uint baudrate, uint rx_pin, uint sck_pin, uint tx_pin, uint cs_pin);