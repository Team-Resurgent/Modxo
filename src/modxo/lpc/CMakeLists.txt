target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lpc_interface.c )
target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lpc_log.c )
target_sources(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/lpc_regs.c )

configure_file(${CMAKE_CURRENT_LIST_DIR}/lpc_comm.in temp/lpc_comm.pio @ONLY)
pico_generate_pio_header(${PROJECT_NAME} temp/lpc_comm.pio)



