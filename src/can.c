#include "can.h"
#include "MCP251XFD.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#define SPI_TIMEOUT_US 5000
// Pointers to fill with RamInfo
#define FIFO_COUNT 2

/* ========== MCP251XFD Initialization Helpers ========== */

eERRORRESULT initialize_CAN(PicoSPI* spi_config, MCP251XFD* device)
{

    // Fill device with HAL configuration
    device->SPI_ChipSelect = spi_config->cs_pin;
    device->SPIClockSpeed = spi_config->clock_speed;
    device->InterfaceDevice = spi_config;
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

    // Statically configured for our project as a whole.
    // THIS STATIC SET SHOULD BE MOVED SOMEWHERE CENTRAL
    // Having to go to each repository that is based off our central
    // stack and updating a crystal frequency is a bad bad idea.
    // it works for now, but beware.
    const MCP251XFD_Config config = {
        .XtalFreq = 4000000,
        .OscFreq = 0,
        .SysclkConfig = MCP251XFD_SYSCLK_IS_CLKIN_MUL_10,
        .SYSCLK_Result = &sysclk_speed,
        .NominalBitrate = 1000000,
        .DataBitrate = 4000000, // Arbitrary and untested. Need to find a safe default - can run up to 8Mbps
        .Bandwidth = MCP251XFD_NO_DELAY,
        .BitTimeStats = &bit_stats,
        .ControlFlags = MCP251XFD_CAN_RESTRICTED_MODE_ON_ERROR | MCP251XFD_CAN_ESI_REFLECTS_ERROR_STATUS | MCP251XFD_CAN_RESTRICTED_RETRANS_ATTEMPTS | MCP251XFD_CANFD_BITRATE_SWITCHING_ENABLE | MCP251XFD_CAN_PROTOCOL_EXCEPT_AS_FORM_ERROR | MCP251XFD_CANFD_USE_ISO_CRC | MCP251XFD_CANFD_DONT_USE_RRS_BIT_AS_SID11,

        .GPIO0PinMode = MCP251XFD_PIN_AS_GPIO0_OUT,
        .GPIO1PinMode = MCP251XFD_PIN_AS_INT1_RX,
        .INTsOutMode = MCP251XFD_PINS_PUSHPULL_OUT,
        .TXCANOutMode = MCP251XFD_PINS_PUSHPULL_OUT,

        .SysInterruptFlags = MCP251XFD_INT_ENABLE_ALL_EVENTS

    };

    // Delay at least 3 more ms before initialization to configure chip clock
    sleep_ms(3);

    eERRORRESULT result = Init_MCP251XFD(device, &config);
    if (result == ERR__FREQUENCY_ERROR) {
        // A frequency error is unrecoverable.
        // The out of range frequency value will be stored in
        // `sysclk_speed`. In the future, it would be nice to log this error.

        // For now, we do nothing and the sysclk_result will get dropped.
        return ERR__FREQUENCY_ERROR;
    } else if (result != ERR_OK) {
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

    // Not using a Transmit Event FIFO
    MCP251XFD_RAMInfos TXQ_ram_info = { 0 };
    MCP251XFD_RAMInfos FIFO_ram_info[FIFO_COUNT - 1] = { { 0 } }; // Will hold singular RX FIFO

    MCP251XFD_FIFO fifo_config[FIFO_COUNT] = {
        { .Name = MCP251XFD_TXQ,
            .Size = MCP251XFD_FIFO_8_MESSAGE_DEEP,
            .Payload = MCP251XFD_PAYLOAD_64BYTE,
            .Attempts = MCP251XFD_UNLIMITED_ATTEMPTS,
            .Priority = MCP251XFD_MESSAGE_TX_PRIORITY16,
            .ControlFlags = MCP251XFD_FIFO_NO_RTR_RESPONSE,
            .InterruptFlags = MCP251XFD_FIFO_TX_ATTEMPTS_EXHAUSTED_INT + MCP251XFD_FIFO_TRANSMIT_FIFO_NOT_FULL_INT,
            .RAMInfos = &TXQ_ram_info },
        { .Name = MCP251XFD_FIFO1,
            .Size = MCP251XFD_FIFO_16_MESSAGE_DEEP,
            .Payload = MCP251XFD_PAYLOAD_64BYTE,
            .Direction = MCP251XFD_RECEIVE_FIFO,
            .ControlFlags = MCP251XFD_FIFO_ADD_TIMESTAMP_ON_RX,
            .InterruptFlags = MCP251XFD_FIFO_OVERFLOW_INT + MCP251XFD_FIFO_RECEIVE_FIFO_NOT_EMPTY_INT,
            .RAMInfos = &FIFO_ram_info[0] }
    };

    eERRORRESULT fifo_result = MCP251XFD_ConfigureFIFOList(device, &fifo_config[0], FIFO_COUNT);
    if (fifo_result != ERR_OK) {
        return fifo_result;
    }

#define FILTER_COUNT 1
    MCP251XFD_Filter filter_config[FILTER_COUNT] = {
        { .Filter = MCP251XFD_FILTER0,
            .EnableFilter = true,
            .AcceptanceID = MCP251XFD_ACCEPT_ALL_MESSAGES,
            .AcceptanceMask = MCP251XFD_ACCEPT_ALL_MESSAGES,
            .Match = MCP251XFD_MATCH_SID_EID,
            .PointTo = MCP251XFD_FIFO1 }
    };

    eERRORRESULT filter_result = MCP251XFD_ConfigureFilterList(device, MCP251XFD_D_NET_FILTER_DISABLE, &filter_config[0], FILTER_COUNT);
    if (filter_result != ERR_OK) {
        return filter_result;
    }

    // Opting to not setup sleep mode yet.

    // Start chip in CAN-FD mode. Configuration is closed.
    // return MCP251XFD_StartCANFD(device);

    return MCP251XFD_RequestOperationMode(device, MCP251XFD_INTERNAL_LOOPBACK_MODE, true);
}

/* ==========  CAN DRIVERS AND HAL FUNCTIONS   ========== */

uint32_t MCP251XFD_GetCurrentMs_Pico(void)
{
    return to_ms_since_boot(get_absolute_time());
}

// Polynomial: 0x8005 (reflected 0xA001), Init = 0xFFFF, XOR out = 0x0000
// Taken from internet. I don't think we'll even use the CRC style so just lgtm.
uint16_t crc16_cms(const uint8_t* data, size_t size)
{
    uint16_t crc = 0xFFFF;

    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001)
                crc = (crc >> 1) ^ 0xA001; // 0xA001 is the reflected form of 0x8005
            else
                crc >>= 1;
        }
    }

    return crc;
}

uint16_t MCP251XFD_ComputeCRC16_Pico(const uint8_t* data, size_t size)
{
    return crc16_cms(data, size);
}

eERRORRESULT MCP251XFD_InterfaceInit_Pico(void* pIntDev, uint8_t chipSelect, const uint32_t sckFreq)
{
    if (pIntDev == NULL)
        return ERR__SPI_PARAMETER_ERROR;
    PicoSPI* spi = (PicoSPI*)pIntDev;

    // Initialize Pico SPI and pins
    spi_init(spi->block, sckFreq);
    gpio_set_function(spi->miso_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi->mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi->sck_pin, GPIO_FUNC_SPI);
    // CS pin setup. HIGH when SPI is not transferring
    gpio_init(chipSelect);
    gpio_set_dir(chipSelect, GPIO_OUT);
    gpio_put(chipSelect, 1);
    // Set according to datasheet
    spi_set_format(spi->block, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    return ERR_OK;
}

eERRORRESULT MCP251XFD_InterfaceTransfer_Pico(void* pIntDev, uint8_t chipSelect, uint8_t* txData, uint8_t* rxData,
    size_t size)
{
    if (pIntDev == NULL)
        return ERR__SPI_PARAMETER_ERROR;
    if (txData == NULL)
        return ERR__SPI_PARAMETER_ERROR;

    // Cast from config ptr
    PicoSPI* spi = (PicoSPI*)pIntDev;

    // Disable interrupts during transfer process
    uint32_t irqs = save_and_disable_interrupts();

    // Set CS low to start transfer
    gpio_put(chipSelect, 0);

    // SPI is a byte for byte protocol
    // In the case of this driver, the length of TX will be length of RX.
    // For most cases, the driver will be padding the TX with zeroes
    // or garbage data so that it gets back the desired RX length.

    // The protocol for the MCP251X family of chips uses SPI as a direct
    // memory bus access for RAM, so this is the lowest level of data.

    for (int i = 0; i < size; i++) {

        absolute_time_t stop_time = make_timeout_time_us(SPI_TIMEOUT_US);
        while (!spi_is_writable(spi->block)) {
            if (get_absolute_time() >= stop_time) {
                gpio_put(chipSelect, 1);
                restore_interrupts(irqs);
                return ERR__SPI_TIMEOUT;
            }
        }

        // Using raw register access to store the TX byte in the data register, sending over SPI
        spi_get_hw(spi->block)->dr = *txData++;

        stop_time = make_timeout_time_us(SPI_TIMEOUT_US);
        while (!spi_is_readable(spi->block)) {
            if (get_absolute_time() >= stop_time) {
                gpio_put(chipSelect, 1);
                restore_interrupts(irqs);
                return ERR__SPI_TIMEOUT;
            }
        }

        // Load lower byte of data register as a byte received
        uint8_t dataRead = (uint8_t)(spi_get_hw(spi->block)->dr & 0x000000FF);
        if (rxData)
            *rxData++ = dataRead;
    }

    gpio_put(chipSelect, 1);
    restore_interrupts(irqs);

    return ERR_OK;
}
