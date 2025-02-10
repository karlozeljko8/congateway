#include "ble_interface.h"
#include "nrf_sdh_ble.h"


static const tstr_ble_if str_ble_if = {
    .ble_init           = ble_stack_init,
    .gap_init           = gap_params_init,
    .gatt_init          = gatt_init,
    .services_init      = services_init,
    .advertising_init   = advertising_init,
    .conn_params_init   = conn_params_init,
    .peer_manager_init  = peer_manager_init,
    .advertising_start  = advertising_start
};

const tstr_ble_if *pstr_ble_if = &str_ble_if;