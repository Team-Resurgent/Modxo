# SPDX short identifier: BSD-2-Clause
# Modxo-only build: upstream carlk3 sources under vendor/no_os_fatfs, SPI + FatFs only.
# Vendor tree is unmodified; this file selects sources and omits SDIO/PIO program.
# hardware_pio is linked only so unmodified sd_card.h can include <hardware/pio.h> (PIO types in SDIO structs).

set(_MODXO_FATFS_ROOT "${CMAKE_CURRENT_LIST_DIR}/vendor/no_os_fatfs")

add_library(modxo_fatfs_sd_spi INTERFACE)

target_compile_definitions(modxo_fatfs_sd_spi INTERFACE
    PICO_MAX_SHARED_IRQ_HANDLERS=8u
)

target_sources(modxo_fatfs_sd_spi INTERFACE
    ${_MODXO_FATFS_ROOT}/ff15/source/ff.c
    ${_MODXO_FATFS_ROOT}/ff15/source/ffsystem.c
    ${_MODXO_FATFS_ROOT}/ff15/source/ffunicode.c
    ${_MODXO_FATFS_ROOT}/sd_driver/sd_card.c
    ${_MODXO_FATFS_ROOT}/sd_driver/sd_timeouts.c
    ${_MODXO_FATFS_ROOT}/sd_driver/SPI/my_spi.c
    ${_MODXO_FATFS_ROOT}/sd_driver/SPI/sd_card_spi.c
    ${_MODXO_FATFS_ROOT}/sd_driver/SPI/sd_spi.c
    ${_MODXO_FATFS_ROOT}/src/crash.c
    ${_MODXO_FATFS_ROOT}/src/crc.c
    ${_MODXO_FATFS_ROOT}/src/f_util.c
    ${_MODXO_FATFS_ROOT}/src/ff_stdio.c
    ${_MODXO_FATFS_ROOT}/src/file_stream.c
    ${_MODXO_FATFS_ROOT}/src/glue.c
    ${_MODXO_FATFS_ROOT}/src/my_debug.c
    ${_MODXO_FATFS_ROOT}/src/my_rtc.c
    ${_MODXO_FATFS_ROOT}/src/util.c
)

target_include_directories(modxo_fatfs_sd_spi INTERFACE
    ${_MODXO_FATFS_ROOT}/ff15/source
    ${_MODXO_FATFS_ROOT}/sd_driver
    ${_MODXO_FATFS_ROOT}/include
)

if(PICO_RISCV)
    set(_MODXO_FATFS_HWDEPS hardware_watchdog)
else()
    set(_MODXO_FATFS_HWDEPS cmsis_core)
endif()

target_link_libraries(modxo_fatfs_sd_spi INTERFACE
    hardware_dma
    hardware_pio
    hardware_spi
    hardware_sync
    pico_aon_timer
    pico_stdlib
    ${_MODXO_FATFS_HWDEPS}
)
