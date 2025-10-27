#include "MCP251XFD.h"
#include "can.h"
#include "canard.h"
#include "cyphal/cyphal_memory.h"
#include "cyphal/cyphal_porting.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

#define SPI_CLK_SPEED 1000000
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS 5
#define PIN_SCK 2
#define PIN_MOSI 3
#define PIN_CAN_INT 6 // GPIO pin connected to the MCP251XFD INT1 pin

// A volatile flag to be set by the ISR
volatile bool can_message_received = false;

// Interrupt Service Routine for CAN message reception
void can_irq_handler(uint gpio, uint32_t events)
{
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_message_received = true;
    }
}

void print_operation_mode(MCP251XFD* can)
{
    eMCP251XFD_OperationMode mode;
    eERRORRESULT result = MCP251XFD_GetActualOperationMode(can, &mode);
    if (result == ERR_OK) {
        printf("Current Operation Mode: ");
        switch (mode) {
        case MCP251XFD_NORMAL_CANFD_MODE:
            printf("Normal CAN-FD\n");
            break;
        case MCP251XFD_SLEEP_MODE:
            printf("Sleep\n");
            break;
        case MCP251XFD_INTERNAL_LOOPBACK_MODE:
            printf("Internal Loopback\n");
            break;
        case MCP251XFD_LISTEN_ONLY_MODE:
            printf("Listen Only\n");
            break;
        case MCP251XFD_CONFIGURATION_MODE:
            printf("Configuration\n");
            break;
        case MCP251XFD_EXTERNAL_LOOPBACK_MODE:
            printf("External Loopback\n");
            break;
        case MCP251XFD_NORMAL_CAN20_MODE:
            printf("Normal CAN 2.0\n");
            break;
        case MCP251XFD_RESTRICTED_OPERATION_MODE:
            printf("Restricted Operation\n");
            break;
        default:
            printf("Unknown\n");
            break;
        }
    } else {
        printf("Failed to get operation mode.\n");
    }
}

void print_error_status(MCP251XFD* can)
{
    uint8_t tec, rec;
    eMCP251XFD_TXRXErrorStatus status;

    if (MCP251XFD_GetTransmitReceiveErrorCountAndStatus(can, &tec, &rec, &status) == ERR_OK) {
        printf("TEC: %3u | REC: %3u | Status: ", tec, rec);
        if (status & MCP251XFD_TX_BUS_OFF_STATE)
            printf("TX_BUS_OFF ");
        if (status & MCP251XFD_TX_BUS_PASSIVE_STATE)
            printf("TX_PASSIVE ");
        if (status & MCP251XFD_TX_WARNING_STATE)
            printf("TX_WARNING ");
        if (status & MCP251XFD_RX_BUS_PASSIVE_STATE)
            printf("RX_PASSIVE ");
        if (status & MCP251XFD_RX_WARNING_STATE)
            printf("RX_WARNING ");
        if (status == 0)
            printf("OK");
        printf("\n");
    }
}

int main(void)
{
    stdio_init_all();

    while (!stdio_usb_connected())
        sleep_ms(100);

    // Setup GPIO interrupt for receiving CAN messages
    gpio_init(PIN_CAN_INT);
    gpio_set_dir(PIN_CAN_INT, GPIO_IN);
    gpio_pull_up(PIN_CAN_INT);
    // The MCP251XFD INT pins are active low, so we trigger on a falling edge.
    gpio_set_irq_enabled_with_callback(PIN_CAN_INT, GPIO_IRQ_EDGE_FALL, true, &can_irq_handler);

    printf("Starting MCP2518FD test...\n");
    PicoSPI spi = {
        .block = SPI_PORT,
        .miso_pin = PIN_MISO,
        .mosi_pin = PIN_MOSI,
        .sck_pin = PIN_SCK,
        .cs_pin = PIN_CS,
        .clock_speed = SPI_CLK_SPEED
    };
    MCP251XFD can;
    eERRORRESULT result = initialize_CAN(&spi, &can);
    const char* reason = ERR_ErrorStrings[result];

    printf("Result of the initialization: %s\n", reason);

    if (result != ERR_OK) {
        printf("Halting due to CAN initialization failure.\n");
        while (true) {
            tight_loop_contents();
        }
    }

    print_operation_mode(&can);

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
        2468, // Subject ID of the message we are subscribing to
        CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
        CANARD_MTU_CAN_FD,
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
            .port_id = 1234, // subject ID
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
        if (can_message_received) {
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