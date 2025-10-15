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

bool cyphal_tx(const CanardFrame *frame)
{
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

void cyphal_rx_poll(CanardInstance *ins)
{
    // Ensure valid CAN device and CanardInstance.
    if (!g_can || !ins)
        return;

    // Query the status of the RX FIFO (FIFO1).
    eMCP251XFD_FIFOstatus rx_fifo_status;
    eERRORRESULT r = MCP251XFD_GetFIFOStatus(g_can, MCP251XFD_FIFO1, &rx_fifo_status);
    if (r != ERR_OK)
        return;  // Hardware not ready or bad status.

    // Loop while the RX FIFO contains messages.
    while (rx_fifo_status & MCP251XFD_RX_FIFO_NOT_EMPTY)
    {
        // Temporary storage for received message.
        MCP251XFD_CANMessage msg;
        uint8_t payload[64];  // Maximum CAN-FD payload size.
        uint32_t timestamp_us = 0;  // Timestamp of the received frame.

        msg.PayloadData = payload;  // Driver writes payload data here.

        // Pull one message from FIFO1 into 'msg'.
        r = MCP251XFD_ReceiveMessageFromFIFO(
                g_can,
                &msg,
                MCP251XFD_PAYLOAD_64BYTE,  // Expecting up to 64-byte CAN-FD payload.
                &timestamp_us,
                MCP251XFD_FIFO1
        );

        // If reading failed, break to avoid infinite looping.
        if (r != ERR_OK)
            break;

        // Convert driver message → Libcanard frame structure.
        CanardFrame rx_frame = {
            .extended_can_id = msg.MessageID,
            .payload = {
                .data = msg.PayloadData,
                .size = MCP251XFD_DLCToByte(msg.DLC, true) // isFD == true
            }
        };

        /*
        Pass the frame to Libcanard for processing.
        interface index = 0 (always 0 in a one-bus setup).
        Last two parameters (NULL):
            1) CanardRxSubscription** out_subscription – NULL; 
                LibCanard would fill *out_subscription with the subscription
                object that matches the frame subject id
            2) CanardRxTransfer* out_transfer – NULL;
                LibCanard would fill *out_transfer with the assembled message
                data (only if rx_frame is the LAST FRAME in a full message)

        The return result is currently ignored (hence the void cast)
        */
        (void)canardRxAccept(ins, timestamp_us, &rx_frame, 0, NULL, NULL);

        // Check FIFO again in case more frames arrived.
        r = MCP251XFD_GetFIFOStatus(g_can, MCP251XFD_FIFO1, &rx_fifo_status);
        if (r != ERR_OK)
            break;
    }
}

uint64_t cyphal_port_get_usec(void)
{
    return time_us_64();
}