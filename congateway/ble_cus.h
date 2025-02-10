#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"


#define CUSTOM_SERVICE_UUID_BASE {0xFE, 0x59, 0xEE, 0xBC, 0x32, 0xD8, 0xE7, 0XAB, \
                                  0XF9, 0X40, 0X72, 0X3D, 0XC3, 0XD0, 0X56, 0XA3}

#define CUSTOM_SERVICE_UUID     0X1400
#define CUSTOM_VALUE_CHAR_UUID  0x1401



typedef struct 
{
    //ble_cus_evt_handler_t           evt_handler;               // Event handler to be called for handling events in the Custom Service.
    uint8_t                         initial_custom_value;      // Initial custom value
    ble_srv_cccd_security_mode_t    custom_value_char_attr_md; // Initial security level for Custom Characteristics attribute
} ble_cus_init_t;

struct ble_cus_s
{
    //ble_cus_evt_handler_t         evt_handler;
    uint16_t                      service_handle;                 /**< Handle of Custom Service (as provided by the BLE stack). */
    ble_gatts_char_handles_t      custom_value_handles;           /**< Handles related to the Custom Value characteristic. */
    uint16_t                      conn_handle;                    /**< Handle of the current connection (as provided by the BLE stack, is BLE_CONN_HANDLE_INVALID if not in a connection). */
    uint8_t                       uuid_type;
};

//Forward declaration of the ble_cus_t type.
typedef struct ble_cus_s ble_cus_t;

/**@brief Macro for defining a ble_cus instane
 * 
 * @param _name Name of the instance
 * @hideinitializer
 */
#define BLE_CUS_DEF(_name)                                                                          \
static ble_cus_t _name;                                                                             \
NRF_SDH_BLE_OBSERVER(_name ## _obs,                                                                 \
                     BLE_HRS_BLE_OBSERVER_PRIO,                                                     \
                     ble_cus_on_ble_evt, &_name)   

static void on_write(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt);
static void on_connect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt);
static void on_disconnect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt);
void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context);
uint32_t ble_cus_init(ble_cus_t *p_cus, ble_cus_init_t *p_cus_init);
static uint32_t custom_value_char_add(ble_cus_t *p_cus, ble_cus_init_t *p_cus_init);


