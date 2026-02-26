
add_library(expansion STATIC ${CMAKE_CURRENT_LIST_DIR}/expansion.c)

target_include_directories(expansion PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(expansion PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(expansion PUBLIC modxo pico_stdlib)
