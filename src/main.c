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

int main(void)
{
    stdio_init_all();

    while (!stdio_usb_connected())
        sleep_ms(100);

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

    // See page 16 of the driver library pdf

    uint8_t frame_counter = 0;
    uint32_t transit_iterator = 0;

    for (;; transit_iterator++) {
        // 1. Prepare the CAN message
        uint8_t payload[8] = { frame_counter, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, 0x00 };
        MCP251XFD_CANMessage message = {
            .MessageID = 0x123,
            .ControlFlags = MCP251XFD_CANFD_FRAME | MCP251XFD_NO_SWITCH_BITRATE | MCP251XFD_STANDARD_MESSAGE_ID,
            .DLC = MCP251XFD_DLC_8BYTE,
            .PayloadData = payload
        };

        // 2. Check if the Transmit Queue has space
        eMCP251XFD_TXQstatus txq_status;
        result = MCP251XFD_GetTXQStatus(&can, &txq_status);

        if (result == ERR_OK && (txq_status & MCP251XFD_TX_FIFO_NOT_FULL)) {
            // 3. If there is space, transmit the message
            result = MCP251XFD_TransmitMessageToTXQ(&can, &message, true); // `true` to flush/request transmission
            if (result == ERR_OK) {
                printf("Successfully sent CAN frame. Counter: %d\n", frame_counter);
                frame_counter++;
            } else {
                printf("Failed to transmit CAN frame. Error: %s\n", ERR_ErrorStrings[result]);
            }
        } else if (result != ERR_OK) {
            printf("Failed to get TXQ status. Error: %s\n", ERR_ErrorStrings[result]);
            break;
        } else {
            printf("TXQ is full, skipping transmission this cycle.\n");
        }

        // 4. Every 5 iterations, check if there are any received messages and print the number of messages in transmit queue
        if (transit_iterator > 0 && transit_iterator % 5 == 0) {
            uint8_t txq_index;
            // Get the index of the next message to be written, which equals the current number of messages
            result = MCP251XFD_GetNextMessageAddressTXQ(&can, NULL, &txq_index);
            if (result == ERR_OK) {
                printf("TXQ Status: %u messages.\n", txq_index);
            } else {
                printf("Failed to get TXQ index. Error: %s\n", ERR_ErrorStrings[result]);
            }

            printf("\n--- Checking for received messages ---\n");

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
                        printf("        0x%02X ", rx_message.PayloadData[i]);
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

        sleep_ms(1000);
    }
}
