#include "MCP251XFD.h"
#include "drivers/mcp251xfd.h"
#include "canard.h"
#include "cyphal/cyphal_memory.h"
#include "cyphal/cyphal_porting.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

// A volatile flag to be set by the ISR
volatile bool can_message_received = false;

// Interrupt Service Routine for CAN message reception
void can_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_message_received = true;
    }
}

int main(void)
{
    stdio_init_all();
    while(!stdio_usb_connected())
        sleep_ms(100);

    // Setup GPIO interrupt for receiving CAN messages
    gpio_init(PIN_CAN_INT);
    gpio_set_dir(PIN_CAN_INT, GPIO_IN);
    gpio_pull_up(PIN_CAN_INT);
    // The MCP251XFD INT pins are active low, so we trigger on a falling edge.
    gpio_set_irq_enabled_with_callback(PIN_CAN_INT, GPIO_IRQ_EDGE_FALL, true, &can_irq_handler);

    printf("Starting MCP2518FD test...\n");
    MCP251XFD can;
    eERRORRESULT result = initialize_CAN(&can_config, &can);
    const char* reason = mcp251xfd_debug_error_reason(result);

    printf("Result of the initialization: %s\n", reason);

    if (result != ERR_OK) {
        printf("Halting due to CAN initialization failure.\n");
        while (true) {
            tight_loop_contents();
        }
    }

    mcp251xfd_debug_print_operation_mode(&can);

    // =====================
    // CYPHAL INITIALIZATION
    // ---------------------

    // Initialize o1heap
    cyphal_memory_init();

    // Attach CAN device to Cyphal
    cyphal_port_init(&can);

    // Create the CanardInstance for this Cyphal node, which
    // encapsulates all the required functions and data structures
    // for Cyphal to work.
    CanardMemoryResource memory = cyphal_memory_resource();
    CanardInstance ins = canardInit(memory);

    // Initialize a TX queue for outgoing frames
    struct CanardTxQueue tx_queue = canardTxInit(32, CANARD_MTU_CAN_FD, memory);

    // EXAMPLE NODE ID (subject to change)
    ins.node_id = 42;

    // Subscription object
    static CanardRxSubscription test_sub;

    int32_t sub_result = canardRxSubscribe(
        &ins,
        CanardTransferKindMessage, // Type of transfer: message (broadcast)
        1234, // KEEP OPPOSITE ON DIFFERENT BOARDS
        CANARD_MTU_CAN_FD, // Correct extent (max payload size)
        CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC, // Correct transfer timeout
        &test_sub);

    if (sub_result < 0) {
        printf("Subscription failed: %ld\n", (long)sub_result);
    }

    // =====================

    uint8_t message_counter = 0;

    for (;;) {

        // ====================
        // SEND TESTING MESSAGE
        // --------------------

        // Example: simple payload of 3 bytes
        uint8_t payload[4] = { message_counter, 0xAA, 0xBB, 0xCC };

        // Build metadata describing this transfer
        struct CanardTransferMetadata metadata = {
            .priority = CanardPriorityNominal,
            .transfer_kind = CanardTransferKindMessage,
            .port_id = 2468, // KEEP OPPOSITE ON DIFFERENT BOARDS
            .remote_node_id = CANARD_NODE_ID_UNSET, // broadcast
            .transfer_id = message_counter++ // must increment per subject
        };

        // Wrap payload into a CanardPayload
        struct CanardPayload pl = {
            .size = sizeof(payload),
            .data = payload
        };

        // Push transfer into TX queue
        uint64_t frames_expired = 0;
        int32_t result = canardTxPush(&tx_queue, &ins,
            cyphal_port_get_usec() + 1000000, // 1-second deadline
            &metadata, pl, 0, &frames_expired);

        if (result < 0) {
            printf("canardTxPush failed: %ld\n", (long)result);
        } else {
            printf("Pushed message with counter: %d\n", message_counter);
        }

        // Send the payload on the bus using the "cyphal_porting" layer
        struct CanardTxQueueItem* item = canardTxPeek(&tx_queue);
        while (item != NULL) {
            cyphal_tx(&item->frame); // your hardware send
            canardTxPop(&tx_queue, item);
            canardTxFree(&tx_queue, &ins, item); // new mandatory free call
            item = canardTxPeek(&tx_queue);
        }

        // ====================

        // ==========================
        // CHECK FOR RECEIVED MESSAGE
        // --------------------------

        // Check if the interrupt has sent the "message received" flag;
        // if so, call the "rx_process" from the "cyphal_porting" layer
        // to receieve the message from the MCP receieve FIFO.
        if (can_message_received || message_counter % 5 == 0) {
            can_message_received = false;
            printf("\n--- RX interrupt triggered ---\n");
            cyphal_rx_process(&ins);
            printf("--- RX drain complete ---\n\n");
        }

        // ==========================

        // Delay between loops
        sleep_ms(500);
    }
}