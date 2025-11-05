#include "canbus.h"
#include "config.h"
#include "drivers/canard.h"
#include "drivers/canard/memory.h"
#include "drivers/mcp251xfd/debug.h"
#include "log.h"
#include "pico/stdlib.h"
#include <uavcan/node/Heartbeat_1_0.h>

static MCP251XFD g_candev;
static struct CanardInstance g_can;
static struct CanardTxQueue g_tx_queue;
static uavcan_node_Health_1_0 g_health = { .value = uavcan_node_Health_1_0_NOMINAL };
static uavcan_node_Mode_1_0 g_mode = { .value = uavcan_node_Mode_1_0_INITIALIZATION };

// Service interrupts and flags
static int can_rx_irq_pin = -1;
volatile bool can_rx_pending = false;
void gpio_irq(uint gpio, uint32_t events)
{
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_rx_pending = true;
    }
}
void init_can_rx_irq(uint pin)
{
    can_rx_irq_pin = pin;
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
    gpio_set_irq_enabled_with_callback(PIN_CAN_INT, GPIO_IRQ_EDGE_FALL, true, gpio_irq);
}

CanbusResult initialize_canbus()
{
    init_can_rx_irq(PIN_CAN_INT);
    eERRORRESULT result = initialize_CAN(&can_config, &g_candev);
    const char* reason = mcp251xfd_debug_error_reason(result);
    LOG_INFO("CAN initialization result: %s", reason);
    if (result != ERR_OK) {
        LOG_FATAL("CAN initialization failed.");
    }

    canard_memory_init();
    struct CanardMemoryResource memory = canard_memory_resource();
    g_can = canardInit(memory);
    g_can.node_id = CAN_ID_RADIO_MODULE; // FIXME: KEEP DIFFERENT ON DIFFERENT BOARDS
    LOG_INFO("LibCanard instance created with node ID %d", g_can.node_id);

    // Initialize a TX queue for outgoing frames
    g_tx_queue = canardTxInit(32, CANARD_MTU_CAN_FD, memory);

    return CANBUS_OK;
}

void canbus_set_node_health(uint8_t health)
{
    g_health.value = health;
}
void canbus_set_node_mode(uint8_t mode)
{
    g_mode.value = mode;
}

void canbus_task()
{
    static absolute_time_t heartbeat_due = 0;

    if (can_rx_pending) {
        canard_mcp_rx_process(&g_candev, &g_can);
        can_rx_pending = false;
    }

    int poll = canardTxPoll(&g_tx_queue, &g_can, to_us_since_boot(get_absolute_time()), &g_candev, canard_mcp_tx_frame, NULL, NULL);
    if (poll < 0)
        LOG_ERROR("canardTxPoll failed with error: %d", poll);

    // Publish next heartbeat when the time has exceeded when the next heartbeat is due
    if (get_absolute_time() > heartbeat_due) {
        CanbusResult heartbeat_pub_result = publish_heartbeat(&g_tx_queue, &g_can, g_health, g_mode);

        if (heartbeat_pub_result == CANBUS_OK) {
            LOG_INFO("Heartbeat successfully published");
        } else {
            LOG_ERROR("Heartbeat FAILED to publish: %s", canbus_result_to_string(heartbeat_pub_result));
        }

        heartbeat_due = make_timeout_time_ms(HEARTBEAT_INTERVAL_MS);
    }
}

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