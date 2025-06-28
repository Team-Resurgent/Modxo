
if(NOT DEFINED MODXO_CUSTOM_USB) # Disable specific descriptors and use default ones
    message(STATUS "CUSTOM USB DISABLED")
    return()
endif()

if(${MODXO_CUSTOM_USB} STREQUAL "MODXO_USB_MSC")
    message(STATUS "Using USB MODXO_USB_MSC configuration")
    add_definitions(-DMODXO_USB_MSC)
    target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MSC_3CDC/msc_interface.c)
    target_sources(modxo PRIVATE ${CMAKE_CURRENT_LIST_DIR}/MSC_3CDC/usb_descriptors.c)
    target_link_libraries(modxo PUBLIC tinyusb_board tinyusb_device)
    target_include_directories(modxo PUBLIC ${CMAKE_CURRENT_LIST_DIR}/MSC_3CDC)
else()
    message(FATAL_ERROR "${MODXO_CUSTOM_USB} Not Supported")
endif()


