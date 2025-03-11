#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define POWER_PIN 29

void uart_send(uint8_t * data);
void uart_init(void);
void gprs_command_handle(void);
void gprs_data_handle(void);


