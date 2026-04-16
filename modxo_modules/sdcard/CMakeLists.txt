
add_library(sdcard STATIC ${CMAKE_CURRENT_LIST_DIR}/sdcard.c)

target_include_directories(sdcard PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(sdcard PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(sdcard PUBLIC modxo modxo_sd_card hardware_i2c pico_stdlib)