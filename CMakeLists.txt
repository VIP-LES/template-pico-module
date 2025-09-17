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

project(module-name C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Initialize the MCP251XFD library
set(CONF_MCP251XFD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/conf)
include(MCP251XFD)


# Add executable. Default name is the project name, version 0.1
add_executable(MODULENAME 
        src/main.c
        src/can.c
)


pico_set_program_name(MODULENAME "Please please please change me!!! (and MODULENAME in CMakeLists.txt)")
pico_set_program_version(MODULENAME "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(MODULENAME 0)
pico_enable_stdio_usb(MODULENAME 1)

# Add required template libraries
target_link_libraries(MODULENAME
        pico_stdlib
        hardware_spi
        MCP251XFD
)

# Add the standard include files to the build
target_include_directories(MODULENAME PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src
        ${CMAKE_CURRENT_LIST_DIR}/conf
)

# Set macro guard to allow error strings from MCP251XFD
target_compile_definitions(MODULENAME PRIVATE USE_ERRORS_STRING)

# Add any user requested libraries
target_link_libraries(MODULENAME
        )

pico_add_extra_outputs(MODULENAME)

