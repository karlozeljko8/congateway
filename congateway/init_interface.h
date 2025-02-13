#include <stdio.h>
#include <stdint.h>
#include "stdbool.h"


typedef enum _tenu_module_type {
    MODULE_POWER_MGMT,
    MODULE_BLE,
    MODULE_WAKE_UP_NFC,
    MODULE_PERIPHERAL,
    MODULE_PN532,
    MODULE_GSM,
    MODULE_END
} tenu_module_type;

typedef void (*init_fn)(void);

typedef struct _tstr_module_init {
    tenu_module_type    module_type;
    init_fn             init;         
    bool                is_enabled;
} tstr_module_init;

tstr_module_init *get_modules_init(void);
extern tstr_module_init *pmodule_init;