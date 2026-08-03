# flashtest - standalone Pico SDK app that reads the flash JEDEC id and
# reports it over USB. Uses the pico-sdk bundled in this repo
# (../external/pico-sdk).

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

# Default to the RP2040 Pico. Build for a Pico 2 (RP2350) with -DPICO_BOARD=pico2.
set(PICO_BOARD pico CACHE STRING "Board type")

# Use the pico-sdk that ships with the Modxo repo, one level up in external/.
set(PICO_SDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/pico-sdk")

# Pull in the Pico SDK (must be before project()).
include(pico_sdk_import.cmake)

project(flashtest C CXX ASM)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

pico_sdk_init()

add_executable(flashtest
    main.c
)

# hardware_flash provides flash_do_cmd() / flash_get_unique_id();
# hardware_sync (pulled in by pico_stdlib) provides save_and_disable_interrupts().
target_link_libraries(flashtest PRIVATE
    pico_stdlib
    hardware_flash
)

# Report over USB serial, not UART.
pico_enable_stdio_usb(flashtest 1)
pico_enable_stdio_uart(flashtest 0)

# Produce the .uf2 (plus map/bin/hex) to drag onto the board.
pico_add_extra_outputs(flashtest)
