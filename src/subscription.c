#include "subscription.h"

#include <uavcan/node/Heartbeat_1_0.h>
#include "drivers/canard/include/drivers/canard.h"
#include "log.h"


// Subscription Initializer Prototypes
static void init_heartbeat_sub(struct CanardInstance* can);

// Subscription Callback Prototypes
static void on_heartbeat(const struct CanardRxTransfer* transfer, void* user_reference);


// Call all subscription initializers which are defined in this file.
// Each subscription initializer must have a corresponding subscription
// callback.
void init_subs(struct CanardInstance* can)
{
    init_heartbeat_sub(can);

    // more calls would go here for other subscription initializers
    // that will be defined.
}

// =========================
// SUBSCRIPTION INITIALIZERS
// =========================

static void init_heartbeat_sub(struct CanardInstance* can) 
{
    CanardSubscriptionCallback cb_heartbeat = {
        .handler = on_heartbeat,
        .user_reference = NULL
    };

    struct CanardRxSubscription sub_heartbeat;

    int8_t sub_result = canardRxSubscribe(
        can,
        CanardTransferKindMessage,
        uavcan_node_Heartbeat_1_0_FIXED_PORT_ID_,
        uavcan_node_Heartbeat_1_0_EXTENT_BYTES_,
        CANARD_DEFAULT_TRANSFER_ID_TIMEOUT_USEC,
        &sub_heartbeat);

    if (sub_result < 0) {
        LOG_ERROR("Failed to subscribe to Heartbeat messages, message %d", sub_result);
    } else {
        LOG_INFO("Subscribed to Heartbeat messages");
    }

    sub_heartbeat.user_reference = &cb_heartbeat;
}




// ======================
// SUBSCRIPTION CALLBACKS
// ======================

static void on_heartbeat(const struct CanardRxTransfer* transfer, void* user_reference)
{
    (void)user_reference;

    uavcan_node_Heartbeat_1_0 msg;
    uint inout = transfer->payload.size;
    if (uavcan_node_Heartbeat_1_0_deserialize_(&msg, transfer->payload.data, &inout) < 0)
        return;

    LOG_INFO("[HB] node=%u uptime=%u mode=%u health=%u",
        transfer->metadata.remote_node_id,
        (unsigned)msg.uptime,
        msg.mode.value,
        msg.health.value);
}