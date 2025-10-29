#include "drivers/mcp251xfd.h"
#include "drivers/mcp251xfd/debug.h"
#include "drivers/canard.h"
#include "config.h"
#include "canard.h"
#include "pico/stdlib.h"
#include "canbus.h"

#include <uavcan/node/Heartbeat_1_0.h>

// Service interrupts and flags
static int can_rx_irq_pin = -1;
volatile bool can_rx_pending = false;
void gpio_irq(uint gpio, uint32_t events) {
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_rx_pending = true;
    }
}
void init_can_rx_irq(uint pin) {
    can_rx_irq_pin = pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(PIN_CAN_INT, GPIO_IRQ_EDGE_FALL, true, gpio_irq);
}

static void on_heartbeat(const struct CanardRxTransfer *transfer, void *user_reference) {
    (void) user_reference;

    uavcan_node_Heartbeat_1_0 msg;
    uint inout = transfer->payload.size;
    if (uavcan_node_Heartbeat_1_0_deserialize_(&msg, transfer->payload.data, &inout) < 0) return;

    printf("[HB] node=%u uptime=%u mode=%u health=%u\n",
           transfer->metadata.remote_node_id,
           (unsigned)msg.uptime,
           msg.mode.value,
           msg.health.value);
}

static uavcan_node_Health_1_0 current_health = {.value = uavcan_node_Health_1_0_NOMINAL};
static uavcan_node_Mode_1_0 current_mode = {.value = uavcan_node_Mode_1_0_OPERATIONAL};

int main(void)
{
    stdio_init_all();
    while(!stdio_usb_connected())
        sleep_ms(100);

    init_can_rx_irq(PIN_CAN_INT);
    

    printf("Starting MCP2518FD test...\n");
    MCP251XFD candev;
    eERRORRESULT result = initialize_CAN(&can_config, &candev);
    const char* reason = mcp251xfd_debug_error_reason(result);
    printf("Result of the initialization: %s\n", reason);
    if (result != ERR_OK) {
        printf("Halting due to CAN initialization failure.\n");
        while (true) {
            tight_loop_contents();
        }
    }

    // libcanard setup
    canard_memory_init();
    struct CanardMemoryResource memory = canard_memory_resource();
    struct CanardInstance can = canardInit(memory);
    // TEST NODE ID (subject to change)
    can.node_id = CAN_ID_SENSOR_MODULE;

    
    // TODO
    // curently the mainloop system does not indicate, log, or respond if there is a clog of messages trying to be sent
    // out. if a device was the only one in the bus, (i.e. TX_PASSIVE), then the txq will fill up and silently drop any 
    // messages after those first 8 frames. While there isnt much to do about handling that, we should note it in the log.
    // Some form of structured cli logging might help. 

    // A good example of this is to turn on one device, let it fill for 10 seconds, then connect the other device. 
    // You'll see that it will dump it's first messages onto the bus immediately and then resume with new messages.
    // This feels like it needs improvement.

    // Easiest solution is to check the return value of publish_heartbeat


    // Initialize a TX queue for outgoing frames
    struct CanardTxQueue tx_queue = canardTxInit(32, CANARD_MTU_CAN_FD, memory);

    CanardSubscriptionCallback cb_heartbeat = {
        .handler = on_heartbeat,
        .user_reference = NULL
    };
    struct CanardRxSubscription sub_heartbeat;
    int8_t sub_result = canardRxSubscribe(
        &can, 
        CanardTransferKindMessage, 
        uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_, 
        uavcan_node_Heartbeat_1_0_EXTENT_BYTES_, 
        CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
        &sub_heartbeat
    );
    sub_heartbeat.user_reference = &cb_heartbeat;


    // Main Loop
    absolute_time_t last_heartbeat = 0;
    #define HEARTBEAT_INTERVAL_MS 1000
    while (true) {

        if (can_rx_pending) {
            // read messages from can and call handlers for subscriptions
            canard_mcp_rx_process(&candev, &can);
            can_rx_pending = false;
        }
        // Pull frames from the queue and send to our interface driver
        int pollin_result = canardTxPoll(&tx_queue, &can, to_us_since_boot(get_absolute_time()), &candev, canard_mcp_tx_frame, NULL, NULL);
        // Should check this output maybe?
        if (get_absolute_time() > last_heartbeat) {
            publish_heartbeat(&tx_queue, &can, current_health, current_mode);
            last_heartbeat = make_timeout_time_ms(HEARTBEAT_INTERVAL_MS);
        }
    }
}