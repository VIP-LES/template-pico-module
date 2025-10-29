#include "canard.h"
#include "canbus.h"
#include "config.h"
#include "errors.h"
#include "interrupt.h"
#include "log.h"
#include "subscription.h"
#include "drivers/canard.h"
#include "drivers/mcp251xfd.h"
#include "drivers/mcp251xfd/debug.h"

#include "pico/stdlib.h"

#include <uavcan/node/Heartbeat_1_0.h>

static uavcan_node_Health_1_0 current_health = { .value = uavcan_node_Health_1_0_NOMINAL };
static uavcan_node_Mode_1_0 current_mode = { .value = uavcan_node_Mode_1_0_OPERATIONAL };

int main(void)
{
    // ===============
    // INITIALIZATIONS
    // ===============

    stdio_init_all();

    // Sleep until USB is connected
    while (!stdio_usb_connected())
        sleep_ms(100);
    LOG_INFO("USB STDIO Connected. Starting application.");

    // Initialize CAN RX interrupts
    init_can_rx_irq(PIN_CAN_INT);
    LOG_INFO("CAN RX IRQ initialized on pin %d", PIN_CAN_INT);

    // Initialize CAN
    LOG_INFO("Starting MCP2518FD test...");
    MCP251XFD candev;
    eERRORRESULT result = initialize_CAN(&can_config, &candev);
    const char* reason = mcp251xfd_debug_error_reason(result);
    LOG_INFO("CAN initialization result: %s", reason);
    if (result != ERR_OK) {
        LOG_FATAL("CAN initialization failed.");
    }

    // Initialize Libcanard
    canard_memory_init();
    struct CanardMemoryResource memory = canard_memory_resource();
    struct CanardInstance can = canardInit(memory);
    can.node_id = CAN_ID_SENSOR_MODULE;
    LOG_INFO("LibCanard instance created with node ID %d", can.node_id);

    // Initialize a TX queue for outgoing frames
    struct CanardTxQueue tx_queue = canardTxInit(32, CANARD_MTU_CAN_FD, memory);

    // Initialize this node's subscriptions
    init_subs(&can);

    
    // ===============
    // MAIN LOOP BELOW
    // ===============

    absolute_time_t heartbeat_due = 0;

    LOG_INFO("Entering main loop...");
    while (true) {

        // If CAN RX is pending, read messages from CAN, call handlers 
        // for this node's subscriptions, and reset CAN RX pending
        if (can_rx_pending()) {
            canard_mcp_rx_process(&candev, &can);
            can_rx_pending_reset();
        }

        // Pull frames from the queue and send to our interface driver
        int pollin_result = canardTxPoll(&tx_queue, &can, to_us_since_boot(get_absolute_time()), &candev, canard_mcp_tx_frame, NULL, NULL);
        if (pollin_result < 0) {
            LOG_ERROR("canardTxPoll failed with error: %d", pollin_result);
        }

        // Publish next heartbeat when the time has exceeded when the next heartbeat is due
        if (get_absolute_time() > heartbeat_due) {
            CanbusResult heartbeat_pub_result = publish_heartbeat(&tx_queue, &can, current_health, current_mode);
 
            if (heartbeat_pub_result == CANBUS_OK) {
                LOG_INFO("Heartbeat successfully published");
            } else {
                LOG_ERROR("Heartbeat FAILED to publish: %s", canbus_result_to_string(heartbeat_pub_result));
            }

            heartbeat_due = make_timeout_time_ms(HEARTBEAT_INTERVAL_MS);
        }
    }
}