#include "software_uart.h"



void uart_tx_init(void) {
    // Set the TX pin as output
    nrf_gpio_cfg_output(UART_TX_PIN);
}

void uart_tx_byte(uint8_t byte, uint32_t bit_delay) {
    // Start bit (low)
    nrf_gpio_pin_clear(UART_TX_PIN);
    nrf_delay_us(BIT_DELAY); // Bit delay for 1 bit duration

    // Send each bit (least significant bit first)
    for (int i = 0; i < 8; i++) {
        if (byte & 0x01) {
            nrf_gpio_pin_set(UART_TX_PIN); // Send 1
        } else {
            nrf_gpio_pin_clear(UART_TX_PIN); // Send 0
        }
        nrf_delay_us(BIT_DELAY); // Wait for the next bit
        byte >>= 1;  // Shift the byte to send the next bit
    }

    // Stop bit (high)
    nrf_gpio_pin_set(UART_TX_PIN);
    nrf_delay_us(BIT_DELAY); // Wait for the stop bit duration
}

// Transmit a buffer
void uart_tx_buffer(const uint8_t* buffer, size_t size, uint32_t bit_delay) {
    for (size_t i = 0; i < size; i++) {
        uart_tx_byte(buffer[i], bit_delay);  // Transmit each byte in the buffer
    }
}