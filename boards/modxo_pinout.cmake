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

#pinout selector
if(${MODXO_PINOUT} MATCHES "official_pico")
    include(boards/official_pico.cmake)
elseif(${MODXO_PINOUT} MATCHES "official_pico2")
    include(boards/official_pico2.cmake)
elseif(${MODXO_PINOUT} MATCHES "yd_rp2040")
    include(boards/yd_rp2040.cmake)
elseif(${MODXO_PINOUT} MATCHES "rp2040_zero_tiny")
    include(boards/rp2040_zero_tiny.cmake)
elseif(${MODXO_PINOUT} MATCHES "xiao_rp2040")
    include(boards/xiao_rp2040.cmake)
elseif(${MODXO_PINOUT} MATCHES "ultra")
    include(boards/ultra.cmake)
else()
    set(MODXO_PINOUT "official_pico")
    message(STATUS "NOTE: Modxo pinout not defined.")
    include(boards/official_pico.cmake)
endif()
message(STATUS "Building for ${MODXO_PINOUT}.")