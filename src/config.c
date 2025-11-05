#include "config.h"

i2c_inst_t* i2c_bus_inst = i2c0;

MCP251XFD_FIFO fifo_configs[] = {
    {
        .Name = MCP251XFD_TXQ,
        .Size = MCP251XFD_FIFO_8_MESSAGE_DEEP,
        .Payload = MCP251XFD_PAYLOAD_64BYTE,
        .Attempts = MCP251XFD_UNLIMITED_ATTEMPTS,
        .Priority = MCP251XFD_MESSAGE_TX_PRIORITY16,
        .ControlFlags = MCP251XFD_FIFO_NO_RTR_RESPONSE,
        .InterruptFlags = MCP251XFD_FIFO_TX_ATTEMPTS_EXHAUSTED_INT + MCP251XFD_FIFO_TRANSMIT_FIFO_NOT_FULL_INT,
    },
    {
        .Name = MCP251XFD_FIFO1,
        .Size = MCP251XFD_FIFO_16_MESSAGE_DEEP,
        .Payload = MCP251XFD_PAYLOAD_64BYTE,
        .Direction = MCP251XFD_RECEIVE_FIFO,
        .ControlFlags = MCP251XFD_FIFO_ADD_TIMESTAMP_ON_RX,
        .InterruptFlags = MCP251XFD_FIFO_OVERFLOW_INT + MCP251XFD_FIFO_RECEIVE_FIFO_NOT_EMPTY_INT,
    }
};

MCP251XFD_Filter filter_configs[] = {
    { .Filter = MCP251XFD_FILTER0,
        .EnableFilter = true,
        .AcceptanceID = MCP251XFD_ACCEPT_ALL_MESSAGES,
        .AcceptanceMask = MCP251XFD_ACCEPT_ALL_MESSAGES,
        .Match = MCP251XFD_MATCH_SID_EID,
        .PointTo = MCP251XFD_FIFO1 }
};

PicoMCPConfig can_config = {
    .spi = {
        .block = spi0,
        .clock_speed = 1000000, // 1 MHz
        .sck_pin = 2,
        .mosi_pin = 3,
        .miso_pin = 4,
        .cs_pin = 5,
    },
    .XtalFreq = 40000000, // 40 MHz
    .OscFreq = 0,
    .SysclkConfig = MCP251XFD_SYSCLK_IS_CLKIN, // Our crystal needs no multiplication,
    .NominalBitrate = 1000000, // 1 MHz,
    .DataBitrate = 4000000, // 4MHz, increase if needed.
    .Bandwidth = MCP251XFD_NO_DELAY, // No delay between sending packets. May cause problems with many messages,
    .ControlFlags = MCP251XFD_CAN_RESTRICTED_MODE_ON_ERROR | MCP251XFD_CAN_ESI_REFLECTS_ERROR_STATUS | MCP251XFD_CAN_RESTRICTED_RETRANS_ATTEMPTS | MCP251XFD_CANFD_BITRATE_SWITCHING_ENABLE | MCP251XFD_CAN_PROTOCOL_EXCEPT_AS_FORM_ERROR | MCP251XFD_CANFD_USE_ISO_CRC | MCP251XFD_CANFD_DONT_USE_RRS_BIT_AS_SID11,
    .InterruptFlags = MCP251XFD_INT_RX_EVENT,
    .operation_mode = MCP251XFD_NORMAL_CANFD_MODE,

    .fifo_configs = fifo_configs,
    .num_fifos = count_of(fifo_configs),

    .filter_configs = filter_configs,
    .num_filters = count_of(filter_configs)
};