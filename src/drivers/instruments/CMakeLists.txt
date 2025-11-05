# src/drivers/instruments/CMakeLists.txt

# Create a new library called driver_instruments
add_library(
    driver_instruments
    STATIC
    # This is the "manager" for all instruments
    instruments.c
    
    # This is the specific implementation for the BMP3 sensor
    bmp3_platform.c
)

# It needs its own "include" directory
target_include_directories(
    driver_instruments
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Link this library to the required pico libraries and our new bmp3 api
target_link_libraries(
    driver_instruments
    pico_stdlib
    hardware_i2c
    driver_bmp3_api # Link to the Bosch driver
)