// src/drivers/instruments/include/drivers/instruments/bmp3_sensor.h
#pragma once
#include "hardware/i2c.h"

/**
 * @brief Initializes the BMP3 sensor.
 * @param i2c_inst The I2C peripheral (e.g., i2c0) to use.
 * @return 0 on success, non-zero on failure.
 */
int bmp3_sensor_init(i2c_inst_t* i2c_inst);

/**
 * @brief Reads the latest data from the BMP3 sensor.
 * Logs data to the console.
 */
void bmp3_sensor_poll(void);