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
cmake_minimum_required(VERSION 3.13...3.27)

set(PICO_BOARD pico CACHE STRING "Board type")

set(PROGRAM_NAME sd-usb)

# Pull in Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(${PROGRAM_NAME})

pico_sdk_init()

add_subdirectory(../../src build)

add_executable(${PROGRAM_NAME}
  hw_config.c
  main.c
  msc_disk.c
  usb_descriptors.c
)
target_link_libraries(${PROGRAM_NAME} PRIVATE
    no-OS-FatFS-SD-SDIO-SPI-RPi-Pico
    pico_stdlib
    tinyusb_additions
    tinyusb_board
    tinyusb_device
)

target_include_directories(${PROGRAM_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}/include
)

# https://datasheets.raspberrypi.com/pico/raspberry-pi-pico-c-sdk.pdf
target_compile_definitions(${PROGRAM_NAME} PUBLIC
    PARAM_ASSERTIONS_ENABLE_ALL=1 
    PICO_MALLOC_PANIC=1
    PICO_USE_STACK_GUARDS=1

    USE_PRINTF
    USE_DBG_PRINTF
)

#target_compile_options(${PROGRAM_NAME} PRIVATE -Werror -Wall -Wextra -Wnull-dereference)
target_compile_options(${PROGRAM_NAME} PUBLIC 
    -Wall 
    -Wextra 
    -Wshadow 
    -Wstack-usage=2048 
    -fanalyzer 
)

target_link_options(${PROGRAM_NAME} PRIVATE -Wl,--print-memory-usage)

pico_enable_stdio_usb(${PROGRAM_NAME} 1)
pico_add_extra_outputs(${PROGRAM_NAME})
