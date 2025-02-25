#include "software_uart.h"



void uart_tx_init(void) {
    nrf_gpio_cfg_output(UART_TX_PIN);
}

static void uart_tx_byte(uint8_t byte) {
    nrf_gpio_pin_clear(UART_TX_PIN);
    nrf_delay_us(BIT_DELAY);

    for (int i = 0; i < 8; i++) {
        if (byte & 0x01) {
            nrf_gpio_pin_set(UART_TX_PIN); 
        } else {
            nrf_gpio_pin_clear(UART_TX_PIN);
        }
        nrf_delay_us(BIT_DELAY);
        byte >>= 1;
    }

    nrf_gpio_pin_set(UART_TX_PIN);
    nrf_delay_us(BIT_DELAY);
}

static void uart_tx_buffer(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        uart_tx_byte(buffer[i]);
    }
}

void uart_log(const char *format, ...){
    char log_buffer[BUFFER_SIZE] = {0};
    va_list args;
    va_start(args, format);
    int log_len = vsnprintf(log_buffer, BUFFER_SIZE, format, args);
    va_end(args);

    uart_tx_buffer((uint8_t *)log_buffer, log_len);
}