// src/drivers/instruments/instruments.c
#include "drivers/instruments/instruments.h"
#include "config.h"
#include "drivers/instruments/bmp3_sensor.h"
#include "hardware/i2c.h"
#include "log.h"
#include "pico/stdlib.h"

// 1 second poll interval, matching the Python code's sleep(1)
#define INSTRUMENTS_POLL_INTERVAL_MS 1000

static absolute_time_t next_poll_time;

void instruments_init(void)
{
    // Initialize the I2C bus
    i2c_init(i2c_bus_inst, I2C_BAUD_RATE);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    LOG_INFO("I2C initialized on SDA=%d, SCL=%d", I2C_SDA_PIN, I2C_SCL_PIN);

    // Initialize the BMP3 sensor
    if (bmp3_sensor_init(i2c_bus_inst) != 0) {
        LOG_ERROR("BMP3 sensor initialization failed!");
    } else {
        LOG_INFO("BMP3 sensor initialized successfully.");
    }

    // TODO: Initialize other sensors here (e.g., tsl2591_sensor_init(i2c_bus_inst))

    next_poll_time = make_timeout_time_ms(INSTRUMENTS_POLL_INTERVAL_MS);
}

void instruments_task(void)
{
    // Check if it's time to poll the sensors
    if (get_absolute_time() > next_poll_time) {

        // Poll the BMP3 sensor
        bmp3_sensor_poll();

        // TODO: Poll other sensors here (e.g., tsl2591_sensor_poll())

        // Schedule the next poll
        next_poll_time = make_timeout_time_ms(INSTRUMENTS_POLL_INTERVAL_MS);
    }
}