#pragma once
#include "hardware/spi.h"
#include "MCP251XFD.h"

/**
 * @brief SPI configuration for the Raspberry Pi Pico.
 *
 * This structure holds the information required to configure an SPI
 * interface for use with the HAL driver initialization function.
 *
 * @typedef PicoSPI
 */
typedef struct
{
    spi_inst_t *block;
    uint8_t mosi_pin;
    uint8_t miso_pin;
    uint8_t sck_pin;
    uint8_t cs_pin;
    uint32_t clock_speed;
} PicoSPI;

typedef struct 
{
    PicoSPI spi;
    uint32_t XtalFreq;
    uint32_t OscFreq;
    eMCP251XFD_CLKINtoSYSCLK SysclkConfig;
    uint32_t NominalBitrate;
    uint32_t DataBitrate;
    eMCP251XFD_Bandwidth Bandwidth;
    setMCP251XFD_CANCtrlFlags ControlFlags;
    setMCP251XFD_InterruptEvents InterruptFlags;

    MCP251XFD_FIFO *fifo_configs;
    size_t num_fifos;

    MCP251XFD_Filter *filter_configs;
    size_t num_filters;

    eMCP251XFD_OperationMode operation_mode;
} PicoMCPConfig;