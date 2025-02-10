#include "sdk_common.h"
#include "ble_srv_common.h"
#include "ble_cus.h"
#include <string.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"

#include "ble_fsm.h"

static void on_write(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt)
{
    fsm_context_t fsm = {
        .current_state  = STATE_IDLE,
        .p_cus          = p_cus,
        .p_ble_evt      = p_ble_evt
    };

    NRF_LOG_INFO("On write event! \r\n");

    const ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;
    // Check if the handle passed with the event matches the Custom Value Characteristic handle.
    if (p_evt_write->handle == p_cus->custom_value_handles.value_handle)
    {
        //nrf_gpio_pin_toggle(LED_4); 

        fsm_run(&fsm, EVENT_DATA_RCVD);

    }

}

static void on_connect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt)
{
    NRF_LOG_INFO("Connected event! \r\n");

    p_cus->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
}

static void on_disconnect(ble_cus_t *p_cus, ble_evt_t const *p_ble_evt)
{
    NRF_LOG_INFO("Disonnected event! \r\n");
    UNUSED_PARAMETER(p_ble_evt);
    p_cus->conn_handle = BLE_CONN_HANDLE_INVALID;
}

void ble_cus_on_ble_evt(ble_evt_t const *p_ble_evt, void *p_context)
{
    ble_cus_t *p_cus = (ble_cus_t *) p_context;

    if (p_cus == NULL || p_ble_evt == NULL)
    {
        return;
    }

    switch(p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            on_connect(p_cus, p_ble_evt);
            break;
        case BLE_GAP_EVT_DISCONNECTED:
            on_disconnect(p_cus, p_ble_evt);
            break;
        case BLE_GATTS_EVT_WRITE:
            on_write(p_cus, p_ble_evt);

        default:
            break;
    }
}


uint32_t ble_cus_init(ble_cus_t *p_cus, ble_cus_init_t *p_cus_init)
{
    uint32_t err_code;
    ble_uuid_t ble_uuid;

    if (p_cus == NULL || p_cus_init == NULL)
    {
        return NRF_ERROR_NULL;
    }

    //Initialize Service structure
    p_cus->conn_handle= BLE_CONN_HANDLE_INVALID;

    //Add Custom Service UUID
    ble_uuid128_t base_uuid = {CUSTOM_SERVICE_UUID_BASE};
    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_cus->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type= p_cus->uuid_type;
    ble_uuid.uuid = CUSTOM_SERVICE_UUID;

    //Add Custom Service declaration to the GATT table
    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid, &p_cus->service_handle);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return custom_value_char_add(p_cus, p_cus_init);   
}

static uint32_t custom_value_char_add(ble_cus_t *p_cus, ble_cus_init_t *p_cus_init)
{
    uint32_t            err_code;
    ble_gatts_char_md_t char_md;
    //ble_gatts_attr_md_t cccd_md; resolve warning - currently unused var
    ble_gatts_attr_t    attr_char_value;
    ble_uuid_t          ble_uuid;
    ble_gatts_attr_md_t attr_md;

    memset(&char_md, 0, sizeof(char_md));
    char_md.char_props.read     = 1;
    char_md.char_props.write    = 1;
    char_md.char_props.notify   = 1;
    char_md.p_char_user_desc    = NULL;
    char_md.p_char_pf           = NULL;
    char_md.p_user_desc_md      = NULL;
    char_md.p_cccd_md           = NULL;
    char_md.p_sccd_md           = NULL;

    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.read_perm   = p_cus_init->custom_value_char_attr_md.read_perm;
    attr_md.write_perm  = p_cus_init->custom_value_char_attr_md.write_perm;
    attr_md.vloc        = BLE_GATTS_VLOC_STACK;
    attr_md.rd_auth     = 0;
    attr_md.wr_auth     = 0;
    attr_md.vlen        = 0;

    ble_uuid.type   = p_cus->uuid_type;
    ble_uuid.uuid   = CUSTOM_VALUE_CHAR_UUID;      

    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid      = &ble_uuid;
    attr_char_value.p_attr_md   = &attr_md;
    attr_char_value.init_len    = sizeof(uint8_t);
    attr_char_value.init_offs   = 0;
    attr_char_value.max_len     = NRF_SDH_BLE_GATT_MAX_MTU_SIZE - 3;

    

    err_code = sd_ble_gatts_characteristic_add(p_cus->service_handle,
                                                &char_md, &attr_char_value,
                                                &p_cus->custom_value_handles);
    if (err_code != NRF_SUCCESS)
    {
        return err_code;
    }

    return NRF_SUCCESS;
}
