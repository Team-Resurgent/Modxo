add_library(superio STATIC ${CMAKE_CURRENT_LIST_DIR}/LPC47M152.c ${CMAKE_CURRENT_LIST_DIR}/uart_16550.c)

pico_enable_stdio_usb(superio 1)
pico_enable_stdio_uart(superio 0)

target_include_directories(superio PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(superio PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(superio PUBLIC modxo)
