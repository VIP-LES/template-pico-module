#pragma once
#include <stdbool.h>
#include "pico/stdlib.h"

void init_can_rx_irq(uint pin);

bool can_rx_pending(void);

void can_rx_pending_reset(void);