#
#SPDX short identifier: BSD-2-Clause
#BSD 2-Clause License
#Copyright (c) 2024, Shalx <Alejandro L. Huitron shalxmva@gmail.com>
#

#Default pin values, so the pinout files can skip them if not used
set(LED_STATUS_PIN PICO_DEFAULT_LED_PIN)

set(LED_STRIP1 31) #Not Used
set(LED_STRIP2 31) #Not Used
set(LED_STRIP3 31) #Not Used
set(LED_STRIP4 31) #Not Used

#pinout selector
set(PROJECT_NAME modxo_${PICO_BOARD})
if(${PICO_BOARD} MATCHES "pico")
    include(boards/official_pico.cmake)
elseif(${PICO_BOARD} MATCHES "yd_rp2040")
    include(boards/yd_rp2040.cmake)
elseif(${PICO_BOARD} MATCHES "waveshare_rp2040_zero")
    include(boards/rp2040_zero.cmake)
elseif(${PICO_BOARD} MATCHES "rp2040_zero_clone")
    include(boards/rp2040_zero.cmake)
elseif(${PICO_BOARD} MATCHES "ultra")
    include(boards/ultra.cmake)
else()
    message(STATUS "NOTE: Modxo pinout not defined. using official pico pinout")
    include(boards/official_pico.cmake)
endif()
message(STATUS "Building for ${MODXO_PINOUT}.")