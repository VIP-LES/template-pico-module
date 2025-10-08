#include "can.h"
#include "can_pico_pins.h"
#include "pico/stdlib.h"
#include <stdio.h>


int main(void) {
    // Initialize UART
    stdio_init_all();

    while (!stdio_usb_connected())
        sleep_ms(100);

    printf("Starting CAN Receiver...\n");


    // Configure SPI and CAN controller
    PicoSPI spi = {
        .block = SPI_PORT,
        .miso_pin = PIN_MISO,
        .mosi_pin = PIN_MOSI,
        .sck_pin  = PIN_SCK,
        .cs_pin   = PIN_CS,
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

    // Main receive loop (polling)
    // This loop runs forever, periodically checking whether any CAN frames
    // have been received and stored in the MCP251XFD’s RX FIFO (FIFO1).
    while (true) {

        // Query the status register for FIFO1 (the receive FIFO).
        // This tells us if any unread CAN messages are waiting.
        eMCP251XFD_FIFOstatus fifo_status;
        result = MCP251XFD_GetFIFOStatus(&can, MCP251XFD_FIFO1, &fifo_status);

        // The function returns ERR_OK if communication with the controller succeeded.
        // We also check if the RX FIFO_NOT_EMPTY flag is set, meaning at least one message is available.
        if (result == ERR_OK && (fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY)) {
            MCP251XFD_CANMessage msg;

            // Allocate a payload buffer where received bytes will be stored.
            // Each CAN-FD frame can carry up to 64 bytes of payload.
            uint8_t payload[64];
            msg.PayloadData = payload;

            uint32_t timestamp;

            // Read one message from FIFO1.
            // MCP251XFD_PAYLOAD_64BYTE tells the driver how large the FIFO payload area is.
            // The function fills in the msg struct and payload buffer.
            result = MCP251XFD_ReceiveMessageFromFIFO(
                &can,
                &msg,
                MCP251XFD_PAYLOAD_64BYTE,
                &timestamp,
                MCP251XFD_FIFO1
            );

            // If reading succeeded, print the contents of the frame.
            if (result == ERR_OK) {

                // Determine if this was a CAN-FD frame (as opposed to classic CAN 2.0)
                // to interpret the DLC (data length code) correctly, then
                // convert the DLC field into an actual number of bytes.
                bool isFD = (msg.ControlFlags & MCP251XFD_CANFD_FRAME);
                uint8_t len = MCP251XFD_DLCToByte(msg.DLC, isFD);

                // Print the received frame in a single line.
                // Includes message ID, length, timestamp, and raw payload bytes.
                printf("RX: ID=0x%03lX | %u bytes | Timestamp=%lu µs | Data=",
                       msg.MessageID, len, timestamp);

                for (int i = 0; i < len; i++) {
                    printf(" %02X", payload[i]);
                }

                printf("\n");

            } else {
                // Print an error if reading failed.
                printf("RX error: %s\n", ERR_ErrorStrings[result]);
            }
        }

         // Delay for 10 milliseconds before checking again.
         sleep_ms(10);

    }
}