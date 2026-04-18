set(LIB_NAME lpc_mem_window)

add_library(${LIB_NAME} STATIC ${CMAKE_CURRENT_LIST_DIR}/${LIB_NAME}.c)

target_include_directories(${LIB_NAME}  PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(${LIB_NAME}  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${LIB_NAME}  PUBLIC modxo pico_rand modxo_sd_card )

# Board file definitions for PSRAM
if(DEFINED PSRAM_CS1 AND (PICO_PLATFORM MATCHES "rp2350*"))
    message(STATUS "RP2350 detected, enabling PSRAM support")
    add_compile_definitions(PSRAM_CS1=${PSRAM_CS1})
    add_compile_definitions(PSRAM_ID=${PSRAM_ID})
    add_compile_definitions(PSRAM_MAX_SELECT_FS64=${PSRAM_MAX_SELECT_FS64})
    add_compile_definitions(PSRAM_MIN_DESELECT_FS=${PSRAM_MIN_DESELECT_FS})
    add_compile_definitions(PSRAM_MAX_SCK_HZ=${PSRAM_MAX_SCK_HZ})
    add_compile_definitions(PSRAM_ENABLE=1)
    target_sources(${LIB_NAME} PRIVATE
        handlers/psram.c
    )
endif()

target_sources(${LIB_NAME} PRIVATE
    handlers/rng.c
    handlers/int_ram.c
    handlers/echo.c
    handlers/sdcard.c
)
