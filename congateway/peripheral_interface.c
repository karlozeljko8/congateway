#include "peripheral_interface.h"


static const tstr_peripheral_if str_peripheral_if =
{
    .rtc_init   = rtc_init,
    .gpio_init  = gpio_config,
    .i2c_init   = i2c_init
};

const tstr_peripheral_if *pstr_peripheral_if = &str_peripheral_if;