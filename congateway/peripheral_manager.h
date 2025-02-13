#include "nrf_rtc.h"
#include "nrf_gpio.h"
#include "nrf.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_twi.h"


void rtc_init(void);
void gpio_config(void);
void i2c_init(void);