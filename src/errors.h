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


static inline const char* canbus_result_to_string(CanbusResult result)
{
    switch (result) {
        case CANBUS_OK: return "CANBUS_OK";
        case CANBUS_ERROR_INVALID_ARGUMENT: return "CANBUS_ERROR_INVALID_ARGUMENT";
        case CANBUS_ERROR_OUT_OF_MEMORY: return "CANBUS_ERROR_OUT_OF_MEMORY";
        case CANBUS_ERROR_DSDL_SERIALIZATION: return "CANBUS_ERROR_DSDL_SERIALIZATION";
        default: return "CANBUS_ERROR_UNKNOWN";
    }
}