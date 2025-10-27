# MCP251XFD Driver
# Adapts the external/MCP251XFD library to the Raspberry Pi Pico platform

cmake_minimum_required(VERSION 3.15)

# Bring in exernal/MCP251XFD library
include(MCP251XFD)

add_library(driver_mcp251xfd mcp251xfd.c platform.c)

target_include_directories(driver_mcp251xfd
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(driver_mcp251xfd
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(driver_mcp251xfd
    PUBLIC
    hardware_spi
    MCP251XFD
)
target_link_libraries(driver_mcp251xfd
    PRIVATE
    pico_time
    pico_stdlib
    hardware_sync
)

target_compile_definitions(driver_mcp251xfd
    PUBLIC
    USE_ERRORS_STRING
)