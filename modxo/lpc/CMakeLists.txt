target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lpc_interface.c)
target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lpc_mem.c)
target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lpc_io.c)
target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lpc_log.c)
target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lpc_regs.c)

configure_file(${CMAKE_CURRENT_LIST_DIR}/lpc_comm.in temp/lpc_comm.pio @ONLY)
pico_generate_pio_header(modxo temp/lpc_comm.pio)
