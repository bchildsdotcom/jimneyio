cmake_minimum_required(VERSION 3.12)

# Change your executable name to something creative!
set(NAME jimneyio) # <-- Name your project/executable here!

include(pico_sdk_import.cmake)
include(pimoroni_pico_import.cmake)

# Gooey boilerplate
project(${NAME} C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the SDK
pico_sdk_init()

# Add your source files
add_executable(${NAME}
    main.cpp # <-- Add source files here!
    jimney.cpp
    inclinometer.cpp
    environment.cpp
)

# Include required libraries
# This assumes `pimoroni-pico` is stored alongside your project
include(common/pimoroni_i2c)
include(common/pimoroni_bus)
include(libraries/pico_graphics/pico_graphics)
include(drivers/st7789/st7789)
include(drivers/rgbled/rgbled)
include(drivers/bme68x/bme68x)
include(drivers/button/button)

# Don't forget to link the libraries you need!
target_link_libraries(${NAME}
    pico_stdlib 
    hardware_spi 
    hardware_pwm 
    hardware_dma 
    rgbled 
    pico_graphics 
    st7789 
    pico_multicore 
    hardware_i2c 
    pimoroni_i2c 
    bme68x 
    button
)

# create map/bin/hex file etc.
pico_add_extra_outputs(${NAME})

# Set up files for the release packages
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.uf2
    ${CMAKE_CURRENT_LIST_DIR}/README.md
    DESTINATION .
)

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
set(CPACK_GENERATOR "ZIP" "TGZ")
include(CPack)
