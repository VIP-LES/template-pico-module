/**
 * @file cyphal_porting.h
 * @brief Hardware abstraction layer between Libcanard and the MCP251XFD CAN driver.
 *
 * This module “ports” Cyphal (Libcanard) to the hardware.
 * It converts between:
 *   - Libcanard's generic types (CanardFrame, CanardInstance)
 *   - The driver types (MCP251XFD_CANMessage)
 *
 * The goal: let the Cyphal logic run platform-independent, while this file
 * handles all the hardware specifics (SPI, FIFOs, interrupts, etc.).
 */

#ifndef CYPHAL_PORTING_H
#define CYPHAL_PORTING_H

#include "canard.h"     // Libcanard API types: CanardFrame, CanardInstance, etc.
#include "drivers/mcp251xfd.h"        // The SPI + driver initialization helpers
#include <stdbool.h>
#include <stddef.h>

typedef struct CanardFrame CanardFrame;
typedef struct CanardInstance CanardInstance;
typedef struct CanardRxTransfer CanardRxTransfer;
typedef struct CanardRxSubscription CanardRxSubscription;

/**
 * @brief Initialize the Cyphal transport layer.
 *
 * Stores a pointer to the MCP251XFD device so that all Cyphal TX/RX functions
 * can access the CAN controller instance.
 *
 * @param can_device Pointer to an already initialized MCP251XFD device.
 */
void cyphal_port_init(MCP251XFD *can_device);

/**
 * @brief Send one frame from Libcanard over the MCP251XFD driver.
 *
 * @param frame Pointer to a populated CanardMutableFrame
 * @return true if successfully queued for transmission; false on failure.
 */
bool cyphal_tx(const struct CanardMutableFrame* frame);

/**
 * @brief Process all pending CAN frames.
 *
 * This function drains the MCP251XFD RX FIFO.
 * It converts each received CAN-FD frame into a Libcanard CanardFrame and passes it
 * to the Cyphal protocol layer for processing.
 *
 * CURRENT IMPLEMENTATION IN main.c:
 *   - The hardware interrupt handler sets a flag when RX data is available.
 *   - The main loop calls this function when it sees the flag is true.
 *
 * This routine performs all SPI reads and protocol handoff outside of the ISR,
 * maintaining a semi-interrupt architecture with bounded latency.
 *
 * @param ins Pointer to the active CanardInstance.
 */
void cyphal_rx_process(CanardInstance *ins);

/**
 * @brief Return current time in microseconds since boot.
 *
 * Used by Cyphal for message timestamps and scheduling.
 *
 * @return Current microsecond timestamp.
 */
uint64_t cyphal_port_get_usec(void);

#endif // CYPHAL_PORTING_H