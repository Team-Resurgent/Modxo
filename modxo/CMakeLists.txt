
add_library(modxo STATIC ${CMAKE_CURRENT_LIST_DIR}/modxo.c ${CMAKE_CURRENT_LIST_DIR}/modxo_queue.c)

target_include_directories(modxo PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(modxo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(modxo PUBLIC pico_stdlib pico_unique_id tinyusb_device tinyusb_board pico_multicore hardware_pio hardware_spi hardware_i2c hardware_flash)
add_subdirectory(lpc)
add_subdirectory(usb)
add_subdirectory(config)
add_subdirectory(data_store)
