#include "drivers/mcp251xfd.h"
#include "../../log.h"
#include "config.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "platform.h"
#include <string.h>

/* ========== MCP251XFD Initialization Helpers ========== */

eERRORRESULT initialize_CAN(PicoMCPConfig* config, MCP251XFD* device)
{
    // Ensure the device structure starts zeroed to avoid garbage DriverConfig or other flags
    memset(device, 0, sizeof(*device));

    // Fill device with HAL configuration
    device->SPI_ChipSelect = config->spi.cs_pin;
    device->SPIClockSpeed = config->spi.clock_speed;
    device->InterfaceDevice = (void*)&config->spi;
    // HAL functions
    device->fnComputeCRC16 = MCP251XFD_ComputeCRC16_Pico;
    device->fnGetCurrentms = MCP251XFD_GetCurrentMs_Pico;
    device->fnSPI_Init = MCP251XFD_InterfaceInit_Pico;
    device->fnSPI_Transfer = MCP251XFD_InterfaceTransfer_Pico;
    // Set default GPIO level to 0 I think?
    device->GPIOsOutLevel = 0;

    // Variables filled by initialization
    uint32_t sysclk_speed;
    MCP251XFD_BitTimeStats bit_stats;

    MCP251XFD_Config mcp_config = {
        .SYSCLK_Result = &sysclk_speed,
        .BitTimeStats = &bit_stats,
        .GPIO0PinMode = MCP251XFD_PIN_AS_GPIO0_OUT,
        .GPIO1PinMode = MCP251XFD_PIN_AS_INT1_RX,
        .INTsOutMode = MCP251XFD_PINS_PUSHPULL_OUT,
        .TXCANOutMode = MCP251XFD_PINS_PUSHPULL_OUT,
    };
    mcp_config.XtalFreq = config->XtalFreq;
    mcp_config.OscFreq = config->OscFreq;
    mcp_config.SysclkConfig = config->SysclkConfig;
    mcp_config.NominalBitrate = config->NominalBitrate;
    mcp_config.DataBitrate = config->DataBitrate;
    mcp_config.Bandwidth = config->Bandwidth;
    mcp_config.ControlFlags = config->ControlFlags;
    mcp_config.SysInterruptFlags = config->InterruptFlags;

    // Delay at least 3 more ms before initialization to configure chip clock
    sleep_ms(3);

    eERRORRESULT result = Init_MCP251XFD(device, &mcp_config);
    if (result == ERR__FREQUENCY_ERROR) {
        // A frequency error is unrecoverable.
        // The out of range frequency value will be stored in
        // `sysclk_speed`. In the future, it would be nice to log this error.

        // For now, we do nothing and the sysclk_result will get dropped.
        LOG_ERROR("Unrecoverable frequency error. SYSCLK out of range: %lu Hz", sysclk_speed);
        return ERR__FREQUENCY_ERROR;
    } else if (result != ERR_OK) {
        LOG_ERROR("Failed to initialize MCP251XFD, error: %d", result);
        return result;
    }

    // Setup timestamp
    eERRORRESULT timestamp_result = MCP251XFD_ConfigureTimeStamp(device,
        true,
        MCP251XFD_TS_CAN20_SOF_CANFD_SOF, // capture on SOF of every frame
        40, // prescaler (40 = 1 µs ticks @ 40 MHz SYSCLK)
        false // We aren't using timestamp super heavily, so no need to interrupt on timer rollover
    );
    if (timestamp_result != ERR_OK) {
        return timestamp_result;
    }

    MCP251XFD_RAMInfos FIFO_ram_info[config->num_fifos]; // Will hold info for fifos
    for (int i = 0; i < config->num_fifos; i++) { // Set RAM Info return pointers to our array
        config->fifo_configs[i].RAMInfos = &FIFO_ram_info[i];
    }

    eERRORRESULT fifo_result = MCP251XFD_ConfigureFIFOList(device, config->fifo_configs, config->num_fifos);
    if (fifo_result != ERR_OK) {
        return fifo_result;
    }

    eERRORRESULT filter_result = MCP251XFD_ConfigureFilterList(device, MCP251XFD_D_NET_FILTER_DISABLE, config->filter_configs, config->num_filters);
    if (filter_result != ERR_OK) {
        return filter_result;
    }

    // Opting to not setup sleep mode yet.

    // Start chip in CAN-FD mode. Configuration is closed.
    return MCP251XFD_RequestOperationMode(device, config->operation_mode, true);
}
