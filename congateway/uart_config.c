#include <stdio.h>
#include <stdint.h>
#include "uart_config.h"




// Define pins (random GPIO numbers)
#define UART1_TX_PIN  6   // Random pin for UART1 TX
#define UART1_RX_PIN  8   // Random pin for UART1 RX
#define UART2_TX_PIN  11  // Random pin for UART2 TX
#define UART2_RX_PIN  12  // Random pin for UART2 RX


nrfx_uarte_t uarte1 = NRFX_UARTE_INSTANCE(1); // UART0
//nrfx_uarte_t uarte2 = NRFX_UARTE_INSTANCE(1); // UARTE1

void uart_init(void)
{
    // UART1 configuration
    nrfx_uarte_config_t uarte1_config = NRFX_UARTE_DEFAULT_CONFIG;
    uarte1_config.baudrate = NRF_UARTE_BAUDRATE_115200;
    uarte1_config.hwfc = NRF_UARTE_HWFC_DISABLED; // No hardware flow control


    uarte1_config.pseltxd = UART1_RX_PIN; // Define RX pin
    uarte1_config.pselrxd = UART1_TX_PIN; // Define TX pin

    nrfx_uarte_init(&uarte1, &uarte1_config, NULL); // Initialize UART1

    // UART2 configuration (if supported by your chip)
    //nrfx_uarte_config_t uarte2_config = NRFX_UARTE_DEFAULT_CONFIG;
    //uarte2_config.baudrate = NRF_UARTE_BAUDRATE_115200;

    //uarte2_config.hwfc = NRF_UARTE_HWFC_DISABLED; // No hardware flow control
    //uarte2_config.pseltxd = UART2_RX_PIN; // Define RX pin
    //uarte2_config.pselrxd = UART2_TX_PIN; // Define TX pin

    //nrfx_uarte_init(&uarte2, &uarte2_config, NULL); // Initialize UART2
}


void uart_send_data(nrfx_uarte_t *uart_instance, const char *data)
{
    nrfx_uarte_tx(uart_instance, (uint8_t *)data, strlen(data)); // Transmit the string
    //nrf_delay_ms(50); // Delay between transmissions
}