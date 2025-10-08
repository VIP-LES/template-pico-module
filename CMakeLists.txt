# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pimoroni_pico_plus2_rp2350 CACHE STRING "Board type")

# add CMake directory in path
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import)

# ====================================================================================

project(template-pico-module C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Initialize the MCP251XFD library
set(CONF_MCP251XFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/conf)
include(MCP251XFD)

# ====================================================================================
# === Transmitter build (main.c) ===
# ====================================================================================

# Add executable. Default name is the project name, version 0.1
add_executable(PICO_CAN_TX
        src/main.c
        src/can.c
)

# Set program name and version
pico_set_program_name(PICO_CAN_TX "PICO CAN Transmitter")
pico_set_program_version(PICO_CAN_TX "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(PICO_CAN_TX 0)
pico_enable_stdio_usb(PICO_CAN_TX 1)

# Add required libraries
target_link_libraries(PICO_CAN_TX
        pico_stdlib
        hardware_spi
        MCP251XFD
)

# Add the standard include files to the build
target_include_directories(PICO_CAN_TX PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/conf
)

# Set macro guard to allow error strings from MCP251XFD
target_compile_definitions(PICO_CAN_TX PRIVATE USE_ERRORS_STRING)

# Add any user requested libraries
target_link_libraries(PICO_CAN_TX
        )

# Add the standard extra outputs (UF2, bin, etc.)
pico_add_extra_outputs(PICO_CAN_TX)

# ====================================================================================
# === Receiver build (receiver_main.c) ===
# ====================================================================================

# Add executable for receiver Pico
add_executable(PICO_CAN_RX
        src/receiver_main.c
        src/can.c
)

# Set program name and version
pico_set_program_name(PICO_CAN_RX "PICO CAN Receiver")
pico_set_program_version(PICO_CAN_RX "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(PICO_CAN_RX 0)
pico_enable_stdio_usb(PICO_CAN_RX 1)

# Add required libraries
target_link_libraries(PICO_CAN_RX
        pico_stdlib
        hardware_spi
        MCP251XFD
)

# Add the standard include files to the build
target_include_directories(PICO_CAN_RX PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/conf
)

# Set macro guard to allow error strings from MCP251XFD
target_compile_definitions(PICO_CAN_RX PRIVATE USE_ERRORS_STRING)

# Add any user requested libraries
target_link_libraries(PICO_CAN_RX
        )

# Add the standard extra outputs (UF2, bin, etc.)
pico_add_extra_outputs(PICO_CAN_RX)