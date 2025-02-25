#include "gsm.h"
#include "nrf_gpio.h"

#include "app_uart.h"
#include "nrf_uart.h"
#include "software_uart.h"
#include "nrf_strerror.h"

#define TX 11
#define RX 12

void gsm_init(void){
    nrf_gpio_cfg_output(POWER_PIN);
}

void gsm_power_enable(void){
    nrf_gpio_pin_set(POWER_PIN);
}

void gsm_power_disable(void){
    nrf_gpio_pin_clear(POWER_PIN);
}

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}
#include "nrf_drv_uart.h"

static uint8_t rx_buffer[1]; // Buffer for receiving 1 byte

static const nrf_drv_uart_t m_uart = NRF_DRV_UART_INSTANCE(0);

static const nrf_drv_uart_config_t m_uart_config = {
    .pseltxd            = TX,           // Replace TX_PIN_NUMBER with the pin number for TX
    .pselrxd            = RX,           // Replace RX_PIN_NUMBER with the pin number for RX
    .pselcts            = NRF_UART_PSEL_DISCONNECTED, // Disable CTS
    .pselrts            = NRF_UART_PSEL_DISCONNECTED, // Disable RTS
    .p_context          = NULL,                    // Context for callbacks, if needed
    .hwfc               = NRF_UART_HWFC_DISABLED,  // No hardware flow control
    .parity             = NRF_UART_PARITY_EXCLUDED,// No parity
    .baudrate           = NRF_UART_BAUDRATE_115200,// Set baud rate to 115200
    .interrupt_priority = APP_IRQ_PRIORITY_LOWEST, // Priority of the UART interrupt
};

void uart_event_handler(nrf_drv_uart_event_t * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_UART_EVT_RX_DONE:
            nrf_drv_uart_tx(&m_uart, rx_buffer, sizeof(rx_buffer));
            nrf_drv_uart_rx(&m_uart, rx_buffer, sizeof(rx_buffer));
            break;

        case NRF_DRV_UART_EVT_TX_DONE:
            // Data transmitted
            // Handle after transmission if needed
            break;

        case NRF_DRV_UART_EVT_ERROR:
            // Handle error
            break;

        default:
            break;
    }
}
void uart_send(uint8_t * data, size_t length)
{
    ret_code_t err_code;

    // Send data
    err_code = nrf_drv_uart_tx(&m_uart, data, length);
    uart_log("uart send: %s err_code", nrf_strerror_get);
    if (err_code != NRF_SUCCESS)
    {
        // Handle error
    }
}

void uart_init(void){
    ret_code_t err_code;
    nrf_drv_uart_uninit(&m_uart);
    // Initialize the UART driver
    err_code = nrf_drv_uart_init(&m_uart, &m_uart_config, uart_event_handler);
    uart_log("uart init: %s err_code", nrf_strerror_get(err_code));
    if (err_code != NRF_SUCCESS)
    {
        // Handle error
        return;
    }

    // Enable RX
    nrf_drv_uart_rx_enable(&m_uart);

    nrf_drv_uart_rx(&m_uart, rx_buffer, sizeof(rx_buffer));


}

