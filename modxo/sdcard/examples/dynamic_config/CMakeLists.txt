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
# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.5)

set(PICO_BOARD pico2 CACHE STRING "Board type")

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(dynamic_config_example C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(../../src build)

# Add executable. Default name is the project name, version 0.1
add_executable(dynamic_config_example 
    main.cpp 
)

target_compile_options(dynamic_config_example PUBLIC 
  -Wall 
  -Wextra 
  -Wshadow 
  -Wstack-usage=2048 
  -fanalyzer 
)

add_compile_definitions(
    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1

    # This program is useless without standard input and output.
    USE_PRINTF
    #USE_DBG_PRINTF
)



set_property(TARGET dynamic_config_example APPEND_STRING PROPERTY LINK_FLAGS 
  "-Wl,--print-memory-usage"
)

pico_set_program_name(dynamic_config_example "dynamic_config_example")
pico_set_program_version(dynamic_config_example "0.1")

# Choose source and destination for standard input and output:
#   See 4.1. Serial input and output on Raspberry Pi Pico in Getting started with Raspberry Pi Pico (https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
#   and 2.7.1. Standard Input/Output (stdio) Support in Raspberry Pi Pico C/C++ SDK (https://datasheets.raspberrypi.org/pico/raspberry-pi-pico-c-sdk.pdf):
pico_enable_stdio_uart(dynamic_config_example 1)
pico_enable_stdio_usb(dynamic_config_example 1)

# Add the standard library and FatFS/SPI to the build
target_link_libraries(dynamic_config_example 
    pico_stdlib
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
)

pico_add_extra_outputs(dynamic_config_example)
