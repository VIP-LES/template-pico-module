# libcanard.cmake
# adapts OpenCyphal's libcanard to CMake

set(LIBCANARD_C "${CMAKE_CURRENT_SOURCE_DIR}/external/libcanard/libcanard/canard.c")
set(LIBCANARD_H "${CMAKE_CURRENT_SOURCE_DIR}/external/libcanard/libcanard/canard.h")
set(CAVL2 "${CMAKE_CURRENT_SOURCE_DIR}/external/libcanard/lib/cavl2")


add_library(cavl2 INTERFACE)
target_include_directories(cavl2 INTERFACE ${CAVL2} )

add_library( libcanard ${LIBCANARD_C} )

target_link_libraries( libcanard cavl2 )

target_include_directories(
    libcanard
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/libcanard/libcanard
)

set_target_properties(libcanard PROPERTIES
    PUBLIC_HEADER "${LIBCANARD_H}"
)