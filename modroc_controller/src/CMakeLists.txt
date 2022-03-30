if (TARGET tinyusb_device)
	#What to build
	file(GLOB FILES *.c)
    add_executable(modroc_controller
            ${FILES}
            )
			
	include_directories(../include)
    
	# What else do we need
    target_link_libraries(modroc_controller pico_stdlib hardware_i2c sensors)

    # enable usb output, disable uart output
    pico_enable_stdio_usb(modroc_controller 1)
    pico_enable_stdio_uart(modroc_controller 0)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(modroc_controller)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building modroc_controller because TinyUSB submodule is not initialized in the SDK")
endif()
