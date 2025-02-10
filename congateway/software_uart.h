#include "nrf_gpio.h"
#include "nrf_delay.h"

#define UART_TX_PIN    30
#define BIT_DELAY      200  // Bit delay in microseconds (adjust as needed for your baud rate)

void uart_tx_init(void);
void uart_tx_byte(uint8_t byte, uint32_t bit_delay);
void uart_tx_buffer(const uint8_t* buffer, size_t size, uint32_t bit_delay);