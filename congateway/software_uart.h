#include "nrf_gpio.h"
#include "nrf_delay.h"
#include <stdarg.h>

#define BUFFER_SIZE 256
#define UART_TX_PIN    30
#define BAUD_RATE 115200
#define BIT_DELAY (1000000 / BAUD_RATE)


void uart_tx_init(void);
static void uart_tx_byte(uint8_t byte);
static void uart_tx_buffer(const uint8_t* buffer, size_t size);
void uart_log(const char *format, ...);