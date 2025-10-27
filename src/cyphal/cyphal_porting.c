/**
 * @file cyphal_porting.c
 * @brief Adapter layer between Libcanard and the MCP251XFD CAN driver.
 *
 * This module translates between Libcanard’s generic frame structures
 * (CanardFrame) and the MCP251XFD driver’s message format. It forwards
 * outgoing frames to the driver and passes received frames up to Libcanard.
 *
 * Architecture:
 *   [Application/Node] ↔ [Libcanard] ↔ (this file) ↔ [MCP251XFD driver] ↔ [Bus]
 */

#include "cyphal_porting.h"
#include "hardware/timer.h"   // For time_us_64()
#include <stdio.h>

// Global pointer to the active MCP251XFD device instance (CAN controller).
static MCP251XFD *g_can = NULL;

void cyphal_port_init(MCP251XFD *can_device)
{
    g_can = can_device;  // Store globally for TX/RX functions.
}

static inline uint8_t MCP251XFD_BytesToDLC(uint8_t n)
{
    if (n <= 8)  return n;
    if (n <= 12) return 9;
    if (n <= 16) return 10;
    if (n <= 20) return 11;
    if (n <= 24) return 12;
    if (n <= 32) return 13;
    if (n <= 48) return 14;
    return 15;  // 64 bytes
}

bool cyphal_tx(const struct CanardMutableFrame* frame) {
    // Ensure valid CAN device and frame pointer.
    if (!g_can || !frame)
        return false;

    // Convert Libcanard’s generic CanardFrame → MCP251XFD_CANMessage
    MCP251XFD_CANMessage msg = {
        .MessageID = (uint32_t)frame->extended_can_id,  // Cyphal always uses extended (29-bit) CAN IDs.
        .DLC = MCP251XFD_BytesToDLC(frame->payload.size),  // Convert byte length → DLC code.
        .ControlFlags = MCP251XFD_CANFD_FRAME | MCP251XFD_STANDARD_MESSAGE_ID,
        .PayloadData = (uint8_t *)frame->payload.data
    };

    // Send the frame immediately into the TX queue (true = flush now).
    eERRORRESULT r = MCP251XFD_TransmitMessageToTXQ(g_can, &msg, true);

    // Return true on success.
    return (r == ERR_OK);
}

void cyphal_rx_process(CanardInstance *ins)
{
    // Ensure valid CAN device and instance
    if (!g_can || !ins)
        return;

    // Check if there is at least one frame to process
    eMCP251XFD_FIFOstatus rx_fifo_status;
    if (MCP251XFD_GetFIFOStatus(g_can, MCP251XFD_FIFO1, &rx_fifo_status) != ERR_OK)
        return;
    if (!(rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY))
        return; // Nothing to process

    for (;;) {

        // Drain one frame from receive FIFO
        MCP251XFD_CANMessage msg;
        uint8_t payload[64];
        uint32_t timestamp_us = 0;
        msg.PayloadData = payload;

        eERRORRESULT r = MCP251XFD_ReceiveMessageFromFIFO(
            g_can, &msg, MCP251XFD_PAYLOAD_64BYTE, &timestamp_us, MCP251XFD_FIFO1);

        // Abort loop on any frame read error.
        if (r != ERR_OK)
            break;

        CanardFrame rx_frame = {
            .extended_can_id = msg.MessageID,
            .payload = { .data = msg.PayloadData,
                        .size = MCP251XFD_DLCToByte(msg.DLC, true) }
        };

        // ======================================
        // TEMPORARY LOGGING OF RECEIVED MESSAGES
        // --------------------------------------

        // Attempt to reassemble one complete Cyphal transfer from the incoming frame.
        // If this frame completes a full transfer, Libcanard fills `transfer` and `subscription`.
        CanardRxTransfer transfer;
        CanardRxSubscription* subscription;
        int32_t result = canardRxAccept(ins, timestamp_us, &rx_frame, 0, &transfer, &subscription);

        if (result >= 0) {
            printf("Received Cyphal transfer on port %u (%zu bytes)\n",
                subscription->port_id, transfer.payload.size);

            const uint8_t* payload_bytes = (const uint8_t*) transfer.payload.data;

            // Interpret the first byte as our test counter (matches TX pattern)
            if (transfer.payload.size > 0) {
                uint8_t counter = payload_bytes[0];
                printf("  Message counter: %u\n", counter);
            }

            // Print full payload contents
            printf("  Payload: ");
            for (size_t i = 0; i < transfer.payload.size; i++) {
                printf("0x%02X ", payload_bytes[i]);
            }
            printf("\n\n");

        } else {
            printf("canardRxAccept() returned error code %ld\n", (long)result);
        }

        // ======================================


        // Re-check FIFO before next iteration
        r = MCP251XFD_GetFIFOStatus(g_can, MCP251XFD_FIFO1, &rx_fifo_status);
        if (r != ERR_OK || !(rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY))
            break;  // FIFO is empty or hardware fault
    }
}

uint64_t cyphal_port_get_usec(void)
{
    return time_us_64();
}