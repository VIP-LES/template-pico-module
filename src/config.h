#pragma once
#include "drivers/mcp251xfd.h"

#define PIN_CAN_INT 6 // GPIO pin connected to the MCP251XFD INT pin

// CAN DEFINITIONS - SUBJECT TO MOVE
#define CAN_ID_RADIO_MODULE 11
#define CAN_ID_SENSOR_MODULE 12

#define CAN_TOPIC_BARO_PRESSURE 135

extern MCP251XFD_FIFO fifo_configs[];

extern MCP251XFD_Filter filter_configs[];

extern PicoMCPConfig can_config;

