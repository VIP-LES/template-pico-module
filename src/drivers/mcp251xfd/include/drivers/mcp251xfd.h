#ifndef CAN_H
#define CAN_H

#include "hardware/spi.h"
#include "../../config.h"
#include "MCP251XFD.h"

/* ========== MCP251XFD Initialization Helpers ========== */



/**
 * @brief Initialize the MCP251XFD CAN device with Raspberry Pi Pico–specific settings.
 *
 * This function configures the MCP251XFD device using the provided SPI interface
 * and applies static configuration values required for operation on the Raspberry Pi Pico.
 *
 * @param[in]  config     Pointer to configuration parameters for the MCP251XFD family of chips.
 * @param[out] device     Pointer to the MCP251XFD device state structure. Must be zero-initialized
 *                        before calling this function. On success, it will be populated with
 *                        the driver’s internal state.
 *
 * @return
 * - @ref ERR_OK if the device was successfully initialized.
 * - One of the other @ref eERRORRESULT values if initialization fails.
 *
 * @note The SPI clock frequency must be less than 80% of the MCP251XFD system clock (SYSCLK).
 */
eERRORRESULT initialize_CAN(PicoMCPConfig *config, MCP251XFD *device);

#endif /* CAN_H */
