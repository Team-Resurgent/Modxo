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

project(unix_like C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(../../src build)

# Add executable. Default name is the project name, version 0.1
add_executable(unix_like 
    main.c
    hw_config.c
)

target_compile_options(unix_like PUBLIC 
    -Wall 
    -Wextra 
    -Wshadow 
    -Wstack-usage=2048 
    -fanalyzer 
)

# https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
target_compile_definitions(unix_like PUBLIC
    PICO_STACK_SIZE=0x1000

    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1

    # This program is useless without standard input and output.
    USE_PRINTF
    USE_DBG_PRINTF
)

# Disable CRC checking for SPI-attached cards.
# add_compile_definitions(SD_CRC_ENABLED=0)


set_property(TARGET unix_like APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(unix_like "unix_like")
pico_set_program_version(unix_like "0.1")

# Choose source and destination for standard input and output:
#   See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
#   and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
pico_enable_stdio_uart(unix_like 1)
pico_enable_stdio_usb(unix_like 1)

# Add the standard library and FatFS/SPI to the build
target_link_libraries(unix_like 
    pico_stdlib
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

# NOTE: for this to work, src\ff15\source\ffconf.h must be removed or renamed.
target_include_directories(no-OS-FatFS-SD-SDIO-SPI-RPi-Pico BEFORE INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

pico_add_extra_outputs(unix_like)
