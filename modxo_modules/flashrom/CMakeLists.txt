add_library(flashrom STATIC ${CMAKE_CURRENT_LIST_DIR}/flashrom.c)

target_include_directories(flashrom PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(flashrom PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include  ${CMAKE_CURRENT_LIST_DIR}../lpc_mem_read/include)
target_link_libraries(flashrom PUBLIC modxo lpc_mem_window)
