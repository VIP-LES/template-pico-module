#include "canard.h"
#include "canbus.h"
#include "config.h"
#include "errors.h"
#include "log.h"

#include "pico/stdlib.h"

#include <uavcan/node/Mode_1_0.h>

#include "hardware/i2c.h"
#include "pico-pmsa003i.h"

void setup_i2c(void) {
    // Initialize I2C0 at 100 kHz
    i2c_init(i2c0, 100 * 1000);

    // Assign GPIO functions for SDA and SCL
    gpio_set_function(4, GPIO_FUNC_I2C);
    gpio_set_function(5, GPIO_FUNC_I2C);

    // Enable pull-ups on both lines
    gpio_pull_up(4);
    gpio_pull_up(5);
}

void pmsa003i_print_data(const pmsa003i_data_t *data) {
    printf("==== PMSA003I Air Quality Data ====\n");
    printf("Standard PM concentrations (μg/m³):\n");
    printf("  PM1.0 : %u\n", data->pm10_standard_ug_m3);
    printf("  PM2.5 : %u\n", data->pm25_standard_ug_m3);
    printf("  PM10  : %u\n", data->pm100_standard_ug_m3);

    printf("\nEnvironmental PM concentrations (μg/m³):\n");
    printf("  PM1.0 : %u\n", data->pm10_environmental_ug_m3);
    printf("  PM2.5 : %u\n", data->pm25_environmental_ug_m3);
    printf("  PM10  : %u\n", data->pm100_environmental_ug_m3);

    printf("\nParticle counts (per 0.1 L):\n");
    printf("  >0.3 µm  : %u\n", data->particulate_03um_per_01L);
    printf("  >0.5 µm  : %u\n", data->particulate_05um_per_01L);
    printf("  >1.0 µm  : %u\n", data->particulate_10um_per_01L);
    printf("  >2.5 µm  : %u\n", data->particulate_25um_per_01L);
    printf("  >5.0 µm  : %u\n", data->particulate_50um_per_01L);
    printf("  >10 µm   : %u\n", data->particulate_100um_per_01L);
    printf("===================================\n");
}

int main() {
    // --- INITIALIZE MODULE ---
    stdio_init_all();

    // Sleep until USB is connected
    while (!stdio_usb_connected())
        sleep_ms(100);
    LOG_INFO("USB STDIO Connected. Starting application.");

    // After finishing initialization, set our mode to operational
    canbus_set_node_mode(uavcan_node_Mode_1_0_OPERATIONAL);


    absolute_time_t heartbeat_due = 0;
    pmsa003i_data_t air_quality_data = {0};

    setup_i2c();
    pmsa003i_init(i2c0);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        canbus_task(); // Must run as often as possible

        // Your looping code goes here
        if (get_absolute_time() > heartbeat_due) {
            pmsa003i_read(&air_quality_data);
            pmsa003i_print_data(&air_quality_data);
            heartbeat_due = make_timeout_time_ms(1000);
        }
    }
}