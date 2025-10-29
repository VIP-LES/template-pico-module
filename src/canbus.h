#pragma once

#include "errors.h"
#include "canard.h"
#include <uavcan/node/Health_1_0.h>
#include <uavcan/node/Mode_1_0.h>

CanbusResult publish_heartbeat(struct CanardTxQueue *const txq, const struct CanardInstance *can, uavcan_node_Health_1_0 health, uavcan_node_Mode_1_0 mode);