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

    while (true) {
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
        } else {
            printf("TXQ is full, skipping transmission this cycle.\n");
        }

        sleep_ms(1000);
    }
}
