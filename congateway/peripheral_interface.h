#include "peripheral_manager.h"


typedef void (*rtc_init_fn)(void);
typedef void (*gpio_init_fn)(void);
typedef void (*i2c_init_fn)(void);

typedef struct _tstr_peripheral_if {
    rtc_init_fn     rtc_init;
    gpio_init_fn    gpio_init;
    i2c_init_fn     i2c_init;

}tstr_peripheral_if;

extern const tstr_peripheral_if *pstr_peripheral_if;
