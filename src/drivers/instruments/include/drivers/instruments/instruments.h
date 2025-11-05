// src/drivers/instruments/include/drivers/instruments/instruments.h
#pragma once

/**
 * @brief Initializes the I2C bus and all connected science sensors.
 * Call this once at startup.
 */
void instruments_init(void);

/**
 * @brief Polls all science sensors.
 * This function is non-blocking and should be called in the main loop.
 * It manages its own timing to read sensors at their configured interval.
 */
void instruments_task(void);