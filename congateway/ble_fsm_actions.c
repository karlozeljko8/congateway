#include <stdint.h>
#include <stdio.h>
#include "sdk_errors.h"
#include "ble_fsm_actions.h"
#include "ble_cus.h"

#include "nrf_log.h"





uint8_t unique_password[] = {0x55, 0x44, 0x33, 0x22};
uint8_t first_time_password[] = {0x00, 0x00, 0x00, 0x00};

static void parse_data(fsm_context_t *fsm , const uint8_t *received_buffer)
{
    if (fsm == NULL || received_buffer == NULL )
    {
        return;
    }
    memcpy(fsm->received_data.serial_number, received_buffer, SERIAL_NUMBER_LEN);
    memcpy(fsm->received_data.password, received_buffer + 6, PASSWORD_LEN);

    
}

static void print_received_data(fsm_context_t *fsm){
    for (uint8_t i = 0; i < SERIAL_NUMBER_LEN; i++)
    {
        NRF_LOG_INFO("Serial number: 0x%02x", (uint8_t* ) fsm->received_data.serial_number[i]);
    }
    for (uint8_t i = 0; i < PASSWORD_LEN; i++)
    {
        NRF_LOG_INFO("Password: 0x%02x", (uint8_t* ) fsm->received_data.password[i]);
    }
}

fsm_event_t action_check_serial_number(fsm_context_t *fsm, fsm_event_t event)
{
    fsm_event_t next_event = EVENT_SERIAL_INVALID;
    uint8_t _tst_serial_num_list[6][6] = {
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x10, 0x11, 0x12, 0x13, 0x14, 0x15},
    };

    for(uint8_t i = 0; i < 6; i++)
    {
        if(memcmp(fsm->received_data.serial_number, _tst_serial_num_list[i], 6) == 0)
        {
            next_event = EVENT_SERIAL_VALID;
            break;
        }
    }    

    return next_event;
}

fsm_event_t action_send_unique_password(fsm_context_t *fsm, fsm_event_t event)
{
    fsm_event_t next_event = EVENT_NO_EVENT;
    ret_code_t ble_status;

    ble_gatts_value_t gatts_value;

    memset(&gatts_value, 0, sizeof(gatts_value));

    gatts_value.len     = sizeof(unique_password);
    gatts_value.p_value = unique_password;

    ble_status = ble_send_data(fsm, &gatts_value);

    if(ble_status != NRF_SUCCESS)
    {
        next_event = EVENT_GO_TO_SLEEP;
    }
    next_event = EVENT_PASSWORD_VALID;

    return next_event;
}

static ret_code_t ble_send_data(fsm_context_t *fsm, ble_gatts_value_t *gatts_value)
{   

    ret_code_t err_code;
    err_code = sd_ble_gatts_value_set(fsm->p_cus->conn_handle, fsm->p_cus->custom_value_handles.value_handle, gatts_value);
    if(err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Failed to set the value. Error: 0x%04X ", err_code);
        return err_code;
    }
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = fsm->p_cus->custom_value_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.offset = 0;
    hvx_params.p_len  = &gatts_value->len;
    hvx_params.p_data = gatts_value->p_value;

    err_code = sd_ble_gatts_hvx(fsm->p_cus->conn_handle, &hvx_params);

    return err_code;
}

static fsm_event_t check_received_password(uint8_t *recv_pass)
{
    fsm_event_t next_event = EVENT_PASSWORD_INVALID;

    if (memcmp(recv_pass, unique_password, PASSWORD_LEN) == 0)
    {
        next_event = EVENT_SUBSEQUENT_CONNECTION;
    }

    if (memcmp(recv_pass, first_time_password, PASSWORD_LEN) == 0)
    {
        next_event = EVENT_FIRST_CONNECTION;
    }
    

    return next_event;
}

fsm_event_t action_check_password(fsm_context_t *fsm, fsm_event_t event)
{
    fsm_event_t next_event = EVENT_NO_EVENT;

    const ble_gatts_evt_write_t *p_evt_write = &fsm->p_ble_evt->evt.gatts_evt.params.write;
    const uint8_t *recv_data = p_evt_write->data;

    parse_data(fsm, recv_data);
    print_received_data(fsm);

    next_event = check_received_password(fsm->received_data.password);
    NRF_LOG_INFO("Next event: %d ", (uint8_t* )next_event);

    return next_event;
}


