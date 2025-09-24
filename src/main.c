#include "MCP251XFD.h"
#include "can.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include <stdint.h>
#include <stdio.h>

#define SPI_CLK_SPEED 1000000
#define SPI_PORT spi0
#define PIN_MISO 4
#define PIN_CS 5
#define PIN_SCK 2
#define PIN_MOSI 3

int main(void)
{
    stdio_init_all();

    while (!stdio_usb_connected())
        sleep_ms(100);

    printf("Starting MCP2518FD test...\n");
    PicoSPI spi = {
        .block = SPI_PORT,
        .miso_pin = PIN_MISO,
        .mosi_pin = PIN_MOSI,
        .sck_pin = PIN_SCK,
        .cs_pin = PIN_CS,
        .clock_speed = SPI_CLK_SPEED
    };
    MCP251XFD can = { 0 };
    eERRORRESULT result = initialize_CAN(&spi, &can);
    const char* reason = ERR_ErrorStrings[result];

    printf("Resut of the initialization: %s\n", reason);
    while (true) {
        tight_loop_contents();
    }
}
