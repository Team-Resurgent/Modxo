# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
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
set(PICO_BOARD pico2 CACHE STRING "Board type")

if(NOT DEFINED MODXO_PINOUT)
    if(PICO_BOARD EQUAL pico2)
        set(MODXO_PINOUT "official_pico2")
    else()
        set(MODXO_PINOUT "official_pico")
    endif()
    message(STATUS "NOTE: Modxo pinout not defined. Using ${MODXO_PINOUT}")
endif()

include(boards/modxo_pinout.cmake)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()


project(${PROJECT_NAME})

if(${DEBUG_PRINTS} MATCHES 1)
    add_definitions( -DDEBUG_PRINTS )
endif()

#add_compile_definitions(START_DELAY) #10s Delay to be able to catch startup messages
#add_compile_definitions(LPC_LOGGING) #10s Delay to be able to catch startup messages

# Creates a pico-sdk subdirectory
pico_sdk_init()

add_executable(${PROJECT_NAME})
pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(modxo)

file(GLOB dirs LIST_DIRECTORIES true modxo_modules/*)
foreach(subdir ${dirs})
    IF(IS_DIRECTORY ${subdir} AND EXISTS ${subdir}/CMakeLists.txt)
        add_subdirectory(${subdir})
    endif()
endforeach()

#Source files
target_sources(${PROJECT_NAME} PRIVATE
    src/main.c
)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(${PROJECT_NAME} PRIVATE
    src/modxo/usb/
)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} PRIVATE
    pico_stdlib
    pico_unique_id
    tinyusb_device
    tinyusb_board
    hardware_pio
    hardware_dma
    pico_multicore
    modxo
    ws2812
    legacy_display
    superio
    flashrom
)

set(PICO_DEFAULT_BINARY_TYPE copy_to_ram)
pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

