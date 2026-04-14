#
# SPDX short identifier: BSD-2-Clause
# BSD 2-Clause License
# Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
#

# Upstream snapshot: vendor/no_os_fatfs (unmodified). SPI-only integration:
# fatfs_spi.cmake + sdio_link_stubs.c (Modxo).

if(SD_CARD_SPI_ENABLE)
    include(${CMAKE_CURRENT_LIST_DIR}/fatfs_spi.cmake)

    add_library(modxo_sd_card STATIC
        ${CMAKE_CURRENT_LIST_DIR}/hw_config.c
        ${CMAKE_CURRENT_LIST_DIR}/sdio_link_stubs.c
    )
    target_include_directories(modxo_sd_card PRIVATE ${CMAKE_BINARY_DIR})
    target_link_libraries(modxo_sd_card PUBLIC modxo_fatfs_sd_spi hardware_i2c)
else()
    add_library(modxo_sd_card INTERFACE)
endif()
