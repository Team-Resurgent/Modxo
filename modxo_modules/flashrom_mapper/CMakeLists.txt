add_library(flashrom_mapper STATIC ${CMAKE_CURRENT_LIST_DIR}/flashrom.c ${CMAKE_CURRENT_LIST_DIR}/mapper.c)

target_include_directories(flashrom_mapper PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(flashrom_mapper PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(flashrom_mapper PUBLIC modxo)
