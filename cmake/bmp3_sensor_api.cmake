# cmake/bmp3_sensor_api.cmake

# Define the path to the submodule
set(BMP3_API_DIR ${CMAKE_SOURCE_DIR}/external/BMP3_SensorAPI)

# Add the library
add_library(
    driver_bmp3_api
    STATIC
    ${BMP3_API_DIR}/bmp3.c
)

# Tell other targets where to find the headers
target_include_directories(
    driver_bmp3_api
    PUBLIC
    ${BMP3_API_DIR}
)

# Link it to the Pico SDK
target_link_libraries(
    driver_bmp3_api
    pico_stdlib
)