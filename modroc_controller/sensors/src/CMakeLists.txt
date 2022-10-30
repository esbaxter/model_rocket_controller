file(GLOB FILES *.c)
link_libraries(pico_stdlib hardware_i2c hardware_spi)
include_directories(../include ../../include)
add_library(sensors ${FILES})
