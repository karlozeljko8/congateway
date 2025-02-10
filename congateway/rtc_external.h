#include "software_uart.h"
#include "nrf_drv_twi.h"

// MCP79402 I2C Address
#define MCP79402_I2C_ADDRESS 0x6F

// MCP79402 Register Addresses
#define RTC_SEC_REG       0x00
#define RTC_MIN_REG       0x01
#define RTC_HOUR_REG      0x02
#define RTC_CONTROL_REG   0x07


void twi_init(void);
bool mcp79402_check(void);
void mcp79402_set_time(uint8_t sec, uint8_t min, uint8_t hour);
void mcp79402_start_oscillator(uint8_t sec);
void mcp79402_read_time(uint8_t *sec, uint8_t *min, uint8_t *hour);