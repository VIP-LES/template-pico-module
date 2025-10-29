#include "canbus.h"
#include "../../log.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <uavcan/node/Heartbeat_1_0.h>

CanbusResult publish_heartbeat(struct CanardTxQueue* const txq, const struct CanardInstance* can, uavcan_node_Health_1_0 health, uavcan_node_Mode_1_0 mode)
{
    static uint8_t heartbeat_tid = 0;
    absolute_time_t now = get_absolute_time();
    absolute_time_t deadline = make_timeout_time_ms(500);

    uavcan_node_Heartbeat_1_0 msg = {
        .uptime = to_ms_since_boot(now),
        .health = health,
        .mode = mode,
        .vendor_specific_status_code = 0,
    };

    uint8_t buffer[uavcan_node_Heartbeat_1_0_SERIALIZATION_BUFFER_SIZE_BYTES_];
    uint inout = sizeof(buffer);
    if (uavcan_node_Heartbeat_1_0_serialize_(&msg, buffer, &inout)) {
        LOG_ERROR("Heartbeat serialization failed");
        return CANBUS_ERROR_DSDL_SERIALIZATION;
    }
    const struct CanardPayload pl = {
        .data = buffer,
        .size = inout
    };

    struct CanardTransferMetadata md = {
        .port_id = uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_,
        .priority = CanardPriorityNominal,
        .transfer_id = heartbeat_tid++,
        .transfer_kind = CanardTransferKindMessage,
        .remote_node_id = CANARD_NODE_ID_UNSET
    };
    uint64_t frames_expired;
    // int32_t result = canardTxPush(txq, can, to_us_since_boot(deadline), &md, pl, to_us_since_boot(now), &frames_expired);
    int32_t result = canardTxPush(txq, can, to_us_since_boot(deadline), &md, pl, 0, &frames_expired);
    if (result < 0) {
        LOG_ERROR("Failed to push Heartbeat to TX queue, error: %ld", result);
        return result; // CanbusResult is derived from canard errors
    }

    if (result > 0) {
        LOG_INFO("Published Heartbeat");
    }

    return CANBUS_OK;
}