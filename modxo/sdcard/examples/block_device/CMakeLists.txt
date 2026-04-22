# == DO NEVER EDIT THE NEXT LINES for Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.5)

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(blkdev C CXX ASM)

# add_compile_options(-Os) # Optimize for size (place before pico_sdk_init)

# Initialise the Pico SDK
pico_sdk_init()

add_subdirectory(../../src build)

# Add executable. Default name is the project name, version 0.1
add_executable(blkdev
        main.c
        hw_config.c
)

# Add the standard library to the build
target_link_libraries(blkdev 
    pico_stdlib
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

# https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
target_compile_definitions(blkdev PUBLIC
    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1

    USE_PRINTF
    #USE_DBG_PRINTF
)

target_compile_options(blkdev PUBLIC 
    -Wall 
    -Wextra 
    -Wshadow 
    -Wstack-usage=2048 
    -fanalyzer 
)

# Disable CRC checking for SPI-attached cards.
# add_compile_definitions(SD_CRC_ENABLED=0)

# Use Pico's LED to show drive activity. 
# Ensure that PICO_DEFAULT_LED_PIN is set correctly.
# Note that Pico W uses GPIO 25 for SPI communication to the CYW43439.
# add_compile_definitions(USE_LED=1)

set_property(TARGET blkdev APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(blkdev "blkdev")
pico_set_program_version(blkdev "0.1")

# See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
# and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
# pico_enable_stdio_uart(blkdev 1)
pico_enable_stdio_usb(blkdev 1)

target_link_libraries(blkdev
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

pico_add_extra_outputs(blkdev)
