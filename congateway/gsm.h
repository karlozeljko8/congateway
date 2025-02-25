#include <stdint.h>
#include <stddef.h>

#define POWER_PIN 29

void uart_send(uint8_t * data, size_t length);
void uart_init(void);