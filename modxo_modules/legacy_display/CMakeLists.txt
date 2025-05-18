
add_library(legacy_display STATIC ${CMAKE_CURRENT_LIST_DIR}/legacy_display.c)

target_include_directories(legacy_display PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(legacy_display PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(legacy_display PUBLIC modxo pico_stdlib)
