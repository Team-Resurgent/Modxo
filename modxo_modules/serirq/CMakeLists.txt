
add_library(serirq STATIC ${CMAKE_CURRENT_LIST_DIR}/serirq.c)

target_include_directories(serirq PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(serirq PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(serirq PUBLIC modxo pico_stdlib hardware_pio)
pico_generate_pio_header(serirq ${CMAKE_CURRENT_LIST_DIR}/serirq.pio)
