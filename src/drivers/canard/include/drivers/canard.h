#pragma once

#include "canard.h"            
#include "canard/memory.h"
#include "MCP251XFD.h"

typedef void (*CanardMessageHandler)(
    const struct CanardRxTransfer *transfer,
    void *user_reference
);

typedef struct {
    CanardMessageHandler handler;
    void *user_reference;
} CanardSubscriptionCallback;

int8_t canard_mcp_tx_frame(void *const mcp251xfd, const CanardMicrosecond deadline_usec, struct CanardMutableFrame *const frame);

uint32_t canard_mcp_rx_process(MCP251XFD *dev, struct CanardInstance *can);