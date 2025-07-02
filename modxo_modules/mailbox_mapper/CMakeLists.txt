add_library(mailbox_mapper STATIC 
    ${CMAKE_CURRENT_LIST_DIR}/flashrom_mailbox.c
    ${CMAKE_CURRENT_LIST_DIR}/mapper.c
    ${CMAKE_CURRENT_LIST_DIR}/lpc_mem.c
)

target_include_directories(mailbox_mapper PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(mailbox_mapper PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(mailbox_mapper PUBLIC modxo)
