#pragma once
#include "drivers/mcp251xfd/include/drivers/mcp251xfd.h"
#include "hardware/i2c.h"

#define PIN_CAN_INT 6 // GPIO pin connected to the MCP251XFD INT pin

// --- I2C DEFINITIONS ---
// Use i2c0 or i2c1
extern i2c_inst_t* i2c_bus_inst;
// TODO: Set these to the actual GPIO pins
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9
#define I2C_BAUD_RATE (400 * 1000) // 400kHz

// CAN DEFINITIONS - SUBJECT TO MOVE
#define CAN_ID_RADIO_MODULE 11
#define CAN_ID_SENSOR_MODULE 12

#define HEARTBEAT_INTERVAL_MS 1000

#define CAN_TOPIC_BARO_PRESSURE 135

extern MCP251XFD_FIFO fifo_configs[];

extern MCP251XFD_Filter filter_configs[];

extern PicoMCPConfig can_config;
