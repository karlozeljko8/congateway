#include "power_mgmt_interface.h"


static const tstr_pwr_mgmt_if str_pwr_mgmt_if = {
    .pwr_mgmt_init      = nrf_pwr_mgmt_init,
    .pwr_mgmt_system_on = nrf_pwr_mgmt_run
};

const tstr_pwr_mgmt_if *pstr_pwr_mgmt_if = &str_pwr_mgmt_if;