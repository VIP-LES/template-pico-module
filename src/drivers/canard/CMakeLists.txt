# libcanard Driver
# Adapts external/libcanard to interface with the Raspberry Pi Pico and MCP2518FD.

cmake_minimum_required(VERSION 3.15)

# Bring in exernal libraries
include(libcanard)
include(o1heap)

add_library(driver_canard canard.c memory.c)

target_include_directories(driver_canard
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_include_directories(driver_canard
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(driver_canard
    PUBLIC
    libcanard
)
target_link_libraries(driver_canard
    PRIVATE
    o1heap
    driver_mcp251xfd
    pico_stdlib
    hardware_sync
)
