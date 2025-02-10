
#include "nrf_pwr_mgmt.h"
#include "sdk_errors.h"

typedef ret_code_t (*pwr_mgmt_init_fn) (void);
typedef void (*pwr_mgmt_system_on_fn) (void);


typedef struct _tstr_pwr_mgmt_if {
    pwr_mgmt_init_fn        pwr_mgmt_init;
    pwr_mgmt_system_on_fn   pwr_mgmt_system_on;

} tstr_pwr_mgmt_if;

extern const tstr_pwr_mgmt_if *pstr_pwr_mgmt_if;