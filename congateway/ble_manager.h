

#include "stdio.h"
#include "stdlib.h"

#include "nrf_log.h"
#include "app_error.h"
#include "sdk_errors.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "ble_cus.h"
#include "nrf_ble_qwr.h"
#include "ble_conn_params.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "bsp.h"
#include "peer_manager_types.h"
#include "nrf_ble_gatt.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "app_timer.h"
#include "ble_advertising.h"


void ble_stack_init(void);
void gap_params_init(void);
void gatt_init(void);
void services_init(void);
void advertising_init(void);
void conn_params_init(void);
void peer_manager_init(void);
void advertising_start(bool erase_bonds);
static void on_adv_evt(ble_adv_evt_t ble_adv_evt);
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context);
static void pm_evt_handler(pm_evt_t const * p_evt);
static void conn_params_error_handler(uint32_t nrf_error);
static void nrf_qwr_error_handler(uint32_t nrf_error);
static void on_conn_params_evt(ble_conn_params_evt_t * p_evt);