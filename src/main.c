#include "drivers/mcp251xfd.h"
#include "drivers/mcp251xfd/debug.h"
#include "config.h"
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

    uint8_t frame_counter = 0;

    for (;;) {
        // 1. Prepare and send a CAN message
        uint8_t payload[8] = { frame_counter, 0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0x00 };
        MCP251XFD_CANMessage message = {
            .MessageID = 0x123,
            .ControlFlags = MCP251XFD_CANFD_FRAME | MCP251XFD_NO_SWITCH_BITRATE | MCP251XFD_STANDARD_MESSAGE_ID,
            .DLC = MCP251XFD_DLC_8BYTE,
            .PayloadData = payload
        };

        result = MCP251XFD_TransmitMessageToTXQ(&can, &message, true); // `true` to flush
        if (result == ERR_OK) {
            printf("Transmitted frame. Counter: %d\n", frame_counter);
            frame_counter++;
        } else {
            const char *reason = mcp251xfd_debug_error_reason(result);
            printf("Failed to transmit CAN frame. Error: %s\n", reason);
        }

        sleep_ms(1000); // Wait a second before sending the next frame
        mcp251xfd_debug_print_error_status(&can);

        // 2. Check if the interrupt fired
        if (can_message_received || frame_counter % 5 == 0) {
            can_message_received = false; // Reset the flag

            printf("\n--- Interrupt triggered! Checking for received messages ---\n");

            eMCP251XFD_FIFOstatus rx_fifo_status;
            result = MCP251XFD_GetFIFOStatus(&can, MCP251XFD_FIFO1, &rx_fifo_status);

            // Keep reading messages as long as the FIFO is not empty
            while (result == ERR_OK && (rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY)) {
                // Prepare to receive the message
                uint8_t rx_payload[64];
                uint32_t timestamp;
                MCP251XFD_CANMessage rx_message;
                rx_message.PayloadData = rx_payload;

                result = MCP251XFD_ReceiveMessageFromFIFO(&can, &rx_message, MCP251XFD_PAYLOAD_64BYTE, &timestamp, MCP251XFD_FIFO1);

                if (result == ERR_OK) {
                    printf("    Message Received from FIFO1!\n");
                    printf("        ID: 0x%lX\n", rx_message.MessageID);
                    printf("        Timestamp: %lu µs\n", timestamp); // Timestamp is in µs based on prescaler

                    bool isFD = (rx_message.ControlFlags & MCP251XFD_CANFD_FRAME);
                    uint8_t dlc_bytes = MCP251XFD_DLCToByte(rx_message.DLC, isFD);
                    printf("        DLC: %d bytes\n", dlc_bytes);

                    printf("        Payload: ");
                    for (int i = 0; i < dlc_bytes; i++) {
                        printf("0x%02X ", rx_message.PayloadData[i]);
                    }
                    printf("\n\n");
                } else {
                    // printf("Failed to receive message. Error: %s\n", ERR_ErrorStrings[result]);
                    break;
                }

                // Check the status again to see if more messages are in the queue
                result = MCP251XFD_GetFIFOStatus(&can, MCP251XFD_FIFO1, &rx_fifo_status);
            }
            if (!(rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY)) {
                printf("RX FIFO1 is empty.\n");
            }

            MCP251XFD_ClearInterruptEvents(&can, MCP251XFD_INT_RX_EVENT);
            irq_clear(PIN_CAN_INT);

            printf("--- Done checking ---\n\n");
        }
    }
}