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
if(${MODXO_PINOUT} MATCHES "official_pico")
    message(STATUS "NOTE: Building for official_pico.")
    include(boards/official_pico.cmake)
elseif(${MODXO_PINOUT} MATCHES "yd_rp2040")
    message(STATUS "NOTE: Building for yd_rp2040.")
    include(boards/yd_rp2040.cmake)
elseif(${MODXO_PINOUT} MATCHES "rp2040_zero")
    message(STATUS "NOTE: Building for rp2040_zero.")
    include(boards/rp2040_zero.cmake)
else()
    message(STATUS "NOTE: Modxo pinout not defined, using official_pico.")
    include(boards/official_pico.cmake)
endif()