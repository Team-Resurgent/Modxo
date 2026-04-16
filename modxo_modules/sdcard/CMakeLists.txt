
add_library(sdcard STATIC ${CMAKE_CURRENT_LIST_DIR}/sdcard.c)

target_include_directories(sdcard PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(sdcard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(sdcard PUBLIC modxo hardware_i2c pico_stdlib)

if(SD_CARD_SPI_ENABLE)
    target_compile_definitions(sdcard PRIVATE MODXO_SDCARD_FATFS=1)
    target_link_libraries(sdcard PUBLIC modxo_sd_card)
endif()
