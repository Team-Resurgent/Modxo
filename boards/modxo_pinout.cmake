#
#SPDX short identifier: BSD-2-Clause
#BSD 2-Clause License
#Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
#

#Default pin values, so the pinout files can skip them if not used
set(LED_STATUS_PIN PICO_DEFAULT_LED_PIN)

set(LED_STRIP1_PWR 31) #Not Used
set(LED_STRIP1 31) #Not Used
set(LED_STRIP2 31) #Not Used
set(LED_STRIP3 31) #Not Used
set(LED_STRIP4 31) #Not Used
set(BOARD_LED_BRIGHTNESS_ADJUST 1) #Default
set(BOARD_FLASH_SIZE 16777216)
set(PICO_PLATFORM rp2040) #Default platform
set(PICO_BOARD pico CACHE STRING "Board type" FORCE ) #Default platform

#pinout selector
set(BOARD_PINOUT_FILE "${CMAKE_CURRENT_LIST_DIR}/${MODXO_PINOUT}.cmake")
if( EXISTS ${BOARD_PINOUT_FILE})
    include(${BOARD_PINOUT_FILE})
else()
    message(FATAL_ERROR "Pinout file not found: ${BOARD_PINOUT_FILE}")
endif()

add_compile_definitions(BOARD_LED_BRIGHTNESS_ADJUST=${BOARD_LED_BRIGHTNESS_ADJUST})
message(STATUS "Building for ${MODXO_PINOUT}.")

configure_file(${CMAKE_CURRENT_LIST_DIR}/modxo_pinout.h.in modxo_pinout.h @ONLY)

add_compile_definitions(PICO_FLASH_SIZE_BYTES=${BOARD_FLASH_SIZE})

#Output filename
set(PROJECT_NAME "modxo_${MODXO_PINOUT}")
