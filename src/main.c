#include "canard.h"
#include "canbus.h"
#include "config.h"
#include "errors.h"
#include "log.h"

#include "pico/stdlib.h"

#include <uavcan/node/Mode_1_0.h>

int main() {
    // --- INITIALIZE MODULE ---
    stdio_init_all();

    // Sleep until USB is connected
    while (!stdio_usb_connected())
        sleep_ms(100);
    LOG_INFO("USB STDIO Connected. Starting application.");

    // After finishing initialization, set our mode to operational
    canbus_set_node_mode(uavcan_node_Mode_1_0_OPERATIONAL);

    // --- MAIN LOOP ---
    LOG_INFO("Entering main loop...");
    while (true) {
        canbus_task(); // Must run as often as possible

        // Your looping code goes here
    }
}