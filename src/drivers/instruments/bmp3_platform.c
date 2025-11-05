// src/drivers/instruments/bmp3_platform.c
#include "../../../config.h"
#include "../../../log.h"
#include "bmp3.h" // From the Bosch driver library
#include "drivers/instruments/bmp3_sensor.h"
#include "pico/stdlib.h"

// The main device structure for the BMP3 driver
static struct bmp3_dev bmp3_device;

// Store the I2C instance
static i2c_inst_t* g_i2c_inst;

// --- Platform-Specific Functions (required by Bosch driver) ---

/**
 * @brief Platform-specific I2C read function
 */
BMP3_INTF_RET_TYPE pico_i2c_read(uint8_t reg_addr, uint8_t* reg_data, uint32_t len, void* intf_ptr)
{
    i2c_inst_t* i2c = (i2c_inst_t*)intf_ptr;

    // BMP3 uses 0x77 (or 0x76) as its 7-bit address
    // The Bosch driver API handles this, but we must write the register address first
    if (i2c_write_blocking(i2c, BMP3_ADDR_I2C_SEC, &reg_addr, 1, true) != 1) {
        return BMP3_E_COMM_FAIL;
    }

    // Read the data
    if (i2c_read_blocking(i2c, BMP3_ADDR_I2C_SEC, reg_data, len, false) != len) {
        return BMP3_E_COMM_FAIL;
    }

    return BMP3_OK;
}

/**
 * @brief Platform-specific I2C write function
 */
BMP3_INTF_RET_TYPE pico_i2c_write(uint8_t reg_addr, const uint8_t* reg_data, uint32_t len, void* intf_ptr)
{
    i2c_inst_t* i2c = (i2c_inst_t*)intf_ptr;

    // Create a temporary buffer to hold the register address followed by the data
    // This is the standard I2C write format
    uint8_t buf[len + 1];
    buf[0] = reg_addr;
    memcpy(buf + 1, reg_data, len);

    if (i2c_write_blocking(i2c, BMP3_ADDR_I2C_SEC, buf, len + 1, false) != (len + 1)) {
        return BMP3_E_COMM_FAIL;
    }

    return BMP3_OK;
}

/**
 * @brief Platform-specific delay function
 */
void pico_delay_us(uint32_t period, void* intf_ptr)
{
    sleep_us(period);
}

// --- Public Functions ---

int bmp3_sensor_init(i2c_inst_t* i2c_inst)
{
    g_i2c_inst = i2c_inst;
    int8_t rslt;

    // Configure the bmp3_dev struct
    bmp3_device.intf_ptr = i2c_inst; // Pass the I2C instance to our read/write functions
    bmp3_device.intf = BMP3_I2C_INTF;
    bmp3_device.read = pico_i2c_read;
    bmp3_device.write = pico_i2c_write;
    bmp3_device.delay_us = pico_delay_us;

    // Initialize the sensor
    rslt = bmp3_init(&bmp3_device);
    if (rslt != BMP3_OK) {
        LOG_ERROR("bmp3_init failed. Code: %d", rslt);
        return rslt;
    }

    // Configure the sensor settings
    // This mimics the default behavior of the Adafruit library (Normal mode, standard oversampling)
    struct bmp3_settings settings = { 0 };
    settings.press_en = BMP3_ENABLE;
    settings.temp_en = BMP3_ENABLE;
    settings.odr_filter.press_os = BMP3_OVERSAMPLING_8X;
    settings.odr_filter.temp_os = BMP3_OVERSAMPLING_2X;
    settings.odr_filter.odr = BMP3_ODR_25_HZ; // ODR must be set for normal mode

    uint16_t settings_select = BMP3_SEL_PRESS_EN | BMP3_SEL_TEMP_EN | BMP3_SEL_PRESS_OS | BMP3_SEL_TEMP_OS | BMP3_SEL_ODR;

    rslt = bmp3_set_sensor_settings(settings_select, &settings, &bmp3_device);
    if (rslt != BMP3_OK) {
        LOG_ERROR("bmp3_set_sensor_settings failed. Code: %d", rslt);
        return rslt;
    }

    // Set the power mode to NORMAL
    settings.op_mode = BMP3_MODE_NORMAL;
    rslt = bmp3_set_op_mode(&settings, &bmp3_device);
    if (rslt != BMP3_OK) {
        LOG_ERROR("bmp3_set_op_mode failed. Code: %d", rslt);
        return rslt;
    }

    return BMP3_OK;
}

void bmp3_sensor_poll(void)
{
    int8_t rslt;
    struct bmp3_data data = { 0 };

    // Get both pressure and temperature data
    rslt = bmp3_get_sensor_data(BMP3_PRESS_TEMP, &data, &bmp3_device);

    if (rslt == BMP3_OK) {
        // The Bosch driver returns data in float/double if BMP3_FLOAT_COMPENSATION is set (default)
        // The Python code reads: temperature, pressure, altitude.
        // The bmp3_get_sensor_data function doesn't calculate altitude, but you can do it.
        // For now, we'll just log the raw data.
        LOG_INFO("BMP3 Data: Temp: %.2f C, Pressure: %.2f hPa", data.temperature, data.pressure / 100.0f);

        // TODO: This is where you would populate your Cyphal/CAN message
        // e.g., publish_baro_pressure(data.pressure);
    } else {
        LOG_ERROR("bmp3_get_sensor_data failed. Code: %d", rslt);
    }
}