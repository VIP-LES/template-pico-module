#include "MCP251XFD.h"
#include "can.h"
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
    // Simply set the flag. The main loop will handle the message processing.
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_message_received = true;
        gpio_pull_down(PIN_CAN_INT);
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
            printf("Failed to transmit CAN frame. Error: %s\n", ERR_ErrorStrings[result]);
        }

        sleep_ms(1000); // Wait a second before sending the next frame

        // 2. Check if the interrupt fired
        if (can_message_received) {
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
                    printf("Failed to receive message. Error: %s\n", ERR_ErrorStrings[result]);
                    break;
                }

                // Check the status again to see if more messages are in the queue
                result = MCP251XFD_GetFIFOStatus(&can, MCP251XFD_FIFO1, &rx_fifo_status);
            }
            if (!(rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY)) {
                printf("RX FIFO1 is empty.\n");
            }
            printf("--- Done checking ---\n\n");
        }
    }
}