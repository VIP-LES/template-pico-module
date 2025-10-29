#pragma once 

#include <stdint.h>

typedef int32_t CanbusResult;

enum
{
    CANBUS_OK = 0,

    CANBUS_ERROR_INVALID_ARGUMENT = -2,
    CANBUS_ERROR_OUT_OF_MEMORY = -3,

    // Payload could not be serialized (buffer too small, value out of range, etc.)
    CANBUS_ERROR_DSDL_SERIALIZATION = -4,
};