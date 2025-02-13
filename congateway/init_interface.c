#include "init_interface.h"

#include "power_mgmt_interface.h"
#include "ble_interface.h"
#include "nfc_interface.h"
#include "peripheral_interface.h"
#include "card_reader_interface.h"
#include "nrf_sdh.h"

void init_power_mgmt(void){
    ret_code_t err_code;

    //err_code = nrf_pwr_mgmt_init();
    err_code = pstr_pwr_mgmt_if->pwr_mgmt_init();
    APP_ERROR_CHECK(err_code);
}

void init_ble(void){
    bool x; // dodati u init strukturu koja se biti argument funkcije init_ble
    pstr_ble_if->ble_init();
    pstr_ble_if->gap_init();         
    pstr_ble_if->gatt_init();        
    pstr_ble_if->services_init();    
    pstr_ble_if->advertising_init(); 
    pstr_ble_if->conn_params_init(); 
    pstr_ble_if->peer_manager_init();
    pstr_ble_if->advertising_start(x);


}
void init_nfc_wakeup(void){
    pstr_nfc_if->nfc_wakeup_config();
}
void init_gpio(void){
    //
}
void init_peripheral(void){
    pstr_peripheral_if->rtc_init();
    pstr_peripheral_if->gpio_init();
    pstr_peripheral_if->i2c_init();
}
void init_gsm(void){

}
void init_pn532(void){
    ret_code_t err_code;
    err_code = pstr_card_reader_if->pn532_init(false);
}

tstr_module_init modules_init[MODULE_END] = {
    {MODULE_POWER_MGMT, init_power_mgmt,    true},
    {MODULE_BLE,        init_ble,           true},
    {MODULE_WAKE_UP_NFC,init_nfc_wakeup,    true},
    {MODULE_PERIPHERAL, init_peripheral,    true},
    {MODULE_PN532,      init_pn532,         true},
    {MODULE_GSM,        init_gsm,           false},
};

tstr_module_init *pmodules_init = modules_init;

tstr_module_init *get_modules_init(void){

    return pmodules_init;
}