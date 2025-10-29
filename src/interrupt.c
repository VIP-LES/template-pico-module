#include "interrupt.h"
#include "config.h"           // for PIN_CAN_INT
#include "hardware/gpio.h"    // for GPIO_IN, GPIO_IRQ_EDGE_FALL
#include "canard.h"

// can_rx_irq_pin currently unused
static int can_rx_irq_pin = -1;
volatile bool can_rx_pending_bool = false;

static void gpio_irq(uint gpio, uint32_t events)
{
    if (gpio == PIN_CAN_INT && (events & GPIO_IRQ_EDGE_FALL)) {
        can_rx_pending_bool = true;
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

bool can_rx_pending(void) {
    return can_rx_pending_bool;
}

void can_rx_pending_reset(void) {
    can_rx_pending_bool = false;
}