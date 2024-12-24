# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

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
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

set(PROJECT_NAME modxo_official_pico)
if(${MODXO_PINOUT} MATCHES "official_pico2")
    set(PROJECT_NAME modxo_official_pico2)
elseif(${MODXO_PINOUT} MATCHES "yd_rp2040")
    set(PROJECT_NAME modxo_yd_rp2040)
elseif(${MODXO_PINOUT} MATCHES "rp2040_zero_tiny")
    set(PROJECT_NAME modxo_rp2040_zero_tiny)
elseif(${MODXO_PINOUT} MATCHES "xiao_rp2040")
    set(PROJECT_NAME modxo_xiao_rp2040)
elseif(${MODXO_PINOUT} MATCHES "ultra")
    set(PROJECT_NAME modxo_ultra)
endif()

project(${PROJECT_NAME})

if(${DEBUG_PRINTS} MATCHES 1)
    add_definitions( -DDEBUG_PRINTS )
endif()
include(boards/modxo_pinout.cmake)
configure_file(${CMAKE_CURRENT_LIST_DIR}/boards/modxo_pinout.h.in modxo_pinout.h @ONLY)

add_compile_definitions(PICO_FLASH_SIZE_BYTES=16777216)
#add_compile_definitions(START_DELAY) #10s Delay to be able to catch startup messages
#add_compile_definitions(LPC_LOGGING) #10s Delay to be able to catch startup messages

# Creates a pico-sdk subdirectory
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

configure_file(${CMAKE_CURRENT_LIST_DIR}/src/modxo/lpc/lpc_comm.in temp/lpc_comm.pio @ONLY)
pico_generate_pio_header(${PROJECT_NAME} "temp/lpc_comm.pio")
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/modxo/ws2812/ws2812.pio)

#Source files
target_sources(${PROJECT_NAME} PRIVATE
    src/modxo/modxo.c
    src/modxo/modxo_queue.c
    src/modxo/modxo_ports.c
    src/modxo/lpc/lpc_comm.in
    src/modxo/lpc/lpc_interface.c
    src/modxo/lpc/lpc_log.c
    src/modxo/lpc/lpc_regs.c
    src/modxo/flashrom/flashrom.c
    src/modxo/data_store/data_store.c
    src/modxo/superio/LPC47M152.c
    src/modxo/superio/uart_16550.c
    src/modxo/ws2812/ws2812.c
    src/modxo/legacy_display/legacy_display.c
    src/modxo/config/config_nvm.c
    src/modxo/config/config_lpc.c
    src/main.c
)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    hardware_pio
    hardware_dma
    pico_multicore
    hardware_i2c
    hardware_spi
    hardware_flash
)

set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

