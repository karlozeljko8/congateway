/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/** @file
 *
 * @defgroup ble_sdk_app_template_main main.c
 * @{
 * @ingroup ble_sdk_app_template
 * @brief Template project main file.
 *
 * This file contains a template for creating a new application. It has the code necessary to wakeup
 * from button, advertise, get a connection restart advertising on disconnect and if no new
 * connection created go back to system-off mode.
 * It can easily be used as a starting point for creating a new application, the comments identified
 * with 'YOUR_JOB' indicates where and how you can customize.
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


#include "init_interface.h"
#include "power_mgmt_interface.h"

#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "nrf_sdh.h"
#include "nrf_sdh_soc.h"
#include "nrf_sdh_ble.h"
#include "app_timer.h"
#include "fds.h"
#include "peer_manager.h"
#include "peer_manager_handler.h"
#include "bsp_btn_ble.h"
#include "sensorsim.h"
#include "ble_conn_state.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf_pwr_mgmt.h"
#include "nrf_delay.h"

#include "nrf_rtc.h"
//#include "nrf52840.h"
//#include "nrf52840_bitfields.h"

#include "adafruit_pn532.h"
#include "nfc_t2t_parser.h"
#include "nfc_t4t_cc_file.h"
#include "nfc_t4t_hl_detection_procedures.h"
#include "nfc_ndef_msg_parser.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "ble_cus.h"


#define DEAD_BEEF                       0xDEADBEEF                              /**< Value used as error code on stack dump, can be used to identify stack location on stack unwind. */

// PN532 Defines
#define SEL_RES_CASCADE_BIT_NUM            3                                              /// Number of Cascade bit within SEL_RES byte.
#define SEL_RES_TAG_PLATFORM_MASK          0x60                                           /// Mask of Tag Platform bit group within SEL_RES byte.
#define SEL_RES_TAG_PLATFORM_BIT_OFFSET    5                                              /// Offset of the Tag Platform bit group within SEL_RES byte.

#define TAG_TYPE_2_UID_LENGTH              7                                              /// Length of the Tag's UID.
#define TAG_TYPE_2_DATA_AREA_SIZE_OFFSET   (T2T_CC_BLOCK_OFFSET + 2)                      /// Offset of the byte with Tag's Data size.
#define TAG_TYPE_2_DATA_AREA_MULTIPLICATOR 8                                              /// Multiplicator for a value stored in the Tag's Data size byte.
#define TAG_TYPE_2_FIRST_DATA_BLOCK_NUM    (T2T_FIRST_DATA_BLOCK_OFFSET / T2T_BLOCK_SIZE) /// First block number with Tag's Data.
#define TAG_TYPE_2_BLOCKS_PER_EXCHANGE     (T2T_MAX_DATA_EXCHANGE / T2T_BLOCK_SIZE)       /// Number of blocks fetched in single Tag's Read command.

#define TAG_TYPE_4_NDEF_FILE_SIZE           255                                           /// Size of the buffer for NDEF file.
#define TAG_TYPE_4_NLEN_FIELD_SIZE          2                                             /// Size of NLEN field inside NDEF file.
#define LED_PIN 28
#define DOOR_OPEN_PIN 27

/**
 * @brief Possible Tag Types.
 */
typedef enum
{
    NFC_T2T = 0x00,      ///< Type 2 Tag Platform.
    NFC_T4T = 0x01,      ///< Type 4A Tag Platform.
    NFC_TT_NOT_SUPPORTED ///< Tag Type not supported.
} nfc_tag_type_t;


/**
 * @brief Macro for handling errors returne by Type 4 Tag modules.
 */
#define T4T_ERROR_HANDLE(ERR_CODE, LOG) \
    if (ERR_CODE != NRF_SUCCESS)        \
    {                                   \
        NRF_LOG_INFO(LOG, ERR_CODE);    \
        return NRF_ERROR_INTERNAL;      \
    }

/**
 * @brief Function for analyzing NDEF data coming either from a Type 2 Tag TLV block or
 *        Type 4 Tag NDEF file.
 */
void ndef_data_analyze(uint8_t * p_ndef_msg_buff, uint32_t nfc_data_len)
{
    ret_code_t err_code;

    uint8_t  desc_buf[NFC_NDEF_PARSER_REQIRED_MEMO_SIZE_CALC(MAX_NDEF_RECORDS)];
    uint32_t desc_buf_len = sizeof(desc_buf);

    err_code = ndef_msg_parser(desc_buf,
                               &desc_buf_len,
                               p_ndef_msg_buff,
                               &nfc_data_len);
    if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error during parsing a NDEF message.");
    }

    ndef_msg_printout((nfc_ndef_msg_desc_t *) desc_buf);
}

/**
 * @brief Function for reading data  from a Type 2 Tag Platform.
 */
ret_code_t t2t_data_read(nfc_a_tag_info * p_tag_info, uint8_t * buffer, uint32_t buffer_size)
{
    ret_code_t err_code;
    uint8_t    block_num = 0;

    // Not enough size in the buffer to read a tag header.
    if (buffer_size < T2T_FIRST_DATA_BLOCK_OFFSET)
    {
        return NRF_ERROR_NO_MEM;
    }

    if (p_tag_info->nfc_id_len != TAG_TYPE_2_UID_LENGTH)
    {
        return NRF_ERROR_NOT_SUPPORTED;
    }

    // Read blocks 0 - 3 to get the header information.
    err_code = adafruit_pn532_tag2_read(block_num, buffer);
    if (err_code)
    {
        NRF_LOG_INFO("Failed to read blocks: %d-%d", block_num,
                     block_num + T2T_END_PAGE_OFFSET);
        return NRF_ERROR_INTERNAL;
    }

    uint16_t data_bytes_in_tag = TAG_TYPE_2_DATA_AREA_MULTIPLICATOR *
                                 buffer[TAG_TYPE_2_DATA_AREA_SIZE_OFFSET];

    if (data_bytes_in_tag + T2T_FIRST_DATA_BLOCK_OFFSET > buffer_size)
    {
        return NRF_ERROR_NO_MEM;
    }

    uint8_t blocks_to_read = data_bytes_in_tag / T2T_BLOCK_SIZE;

    for (block_num = TAG_TYPE_2_FIRST_DATA_BLOCK_NUM;
         block_num < blocks_to_read;
         block_num += TAG_TYPE_2_BLOCKS_PER_EXCHANGE)
    {
        uint16_t offset_for_block = T2T_BLOCK_SIZE * block_num;
        err_code = adafruit_pn532_tag2_read(block_num, buffer + offset_for_block);
        if (err_code)
        {
            NRF_LOG_INFO("Failed to read blocks: %d-%d",
                         block_num,
                         block_num + T2T_END_PAGE_OFFSET);
            return NRF_ERROR_INTERNAL;
        }
    }

    return NRF_SUCCESS;
}


/**
 * @brief Function for analyzing data from a Type 2 Tag Platform.
 *
 * This function parses content of a Type 2 Tag Platform and prints it out.
 */
void t2t_data_analyze(uint8_t * buffer)
{
    ret_code_t err_code;

    // Static declaration of Type 2 Tag structure.
    NFC_TYPE_2_TAG_DESC_DEF(test_1, MAX_TLV_BLOCKS);
    type_2_tag_t * test_type_2_tag = &NFC_TYPE_2_TAG_DESC(test_1);

    err_code = type_2_tag_parse(test_type_2_tag, buffer);
    if (err_code == NRF_ERROR_NO_MEM)
    {
        NRF_LOG_INFO("Not enough memory to read whole tag. Printing what've been read.");
    }
    else if (err_code != NRF_SUCCESS)
    {
        NRF_LOG_INFO("Error during parsing a tag. Printing what could've been read.");
    }

    type_2_tag_printout(test_type_2_tag);

    tlv_block_t * p_tlv_block = test_type_2_tag->p_tlv_block_array;
    uint32_t      i;

    for (i = 0; i < test_type_2_tag->tlv_count; i++)
    {
        if (p_tlv_block->tag == TLV_NDEF_MESSAGE)
        {
            ndef_data_analyze(p_tlv_block->p_value, p_tlv_block->length);
            p_tlv_block++;
        }
    }
}


/**
 * @brief Function for reading and analyzing data from a Type 2 Tag Platform.
 *
 * This function reads content of a Type 2 Tag Platform, parses it and prints it out.
 */
ret_code_t t2t_data_read_and_analyze(nfc_a_tag_info * p_tag_info)
{
    ret_code_t     err_code;
    static uint8_t t2t_data[TAG_TYPE_2_DATA_BUFFER_SIZE]; // Buffer for tag data.

    err_code = t2t_data_read(p_tag_info, t2t_data, TAG_TYPE_2_DATA_BUFFER_SIZE);
    VERIFY_SUCCESS(err_code);

    t2t_data_analyze(t2t_data);

    return NRF_SUCCESS;
}


/**
 * @brief Function for reading and analyzing data from a Type 4 Tag Platform.
 *
 * This function reads content of a Type 4 Tag Platform, parses it and prints it out.
 */
ret_code_t t4t_data_read_and_analyze(nfc_a_tag_info * p_tag_info)
{
    ret_code_t err_code;

    // Static declaration of Type 4 Tag structure.
    NFC_T4T_CC_DESC_DEF(cc_file, MAX_TLV_BLOCKS);
    static uint8_t ndef_files_buffs[MAX_TLV_BLOCKS][TAG_TYPE_4_NDEF_FILE_SIZE];

    err_code = nfc_t4t_ndef_tag_app_select();
    T4T_ERROR_HANDLE(err_code, "Error (0x%X) during NDEF Tag Application Select Procedure.");

    err_code = nfc_t4t_cc_select();
    T4T_ERROR_HANDLE(err_code, "Error (0x%X) during CC Select Procedure.");

    nfc_t4t_capability_container_t * cc_file = &NFC_T4T_CC_DESC(cc_file);
    err_code = nfc_t4t_cc_read(cc_file);
    T4T_ERROR_HANDLE(err_code, "Error (0x%X) during CC Read Procedure.");

    nfc_t4t_tlv_block_t * p_tlv_block = cc_file->p_tlv_block_array;
    uint32_t              i;

    for (i = 0; i < cc_file->tlv_count; i++)
    {
        if ((p_tlv_block->type == NDEF_FILE_CONTROL_TLV) ||
            (p_tlv_block->value.read_access == CONTROL_FILE_READ_ACCESS_GRANTED))
        {
            err_code = nfc_t4t_file_select(p_tlv_block->value.file_id);
            T4T_ERROR_HANDLE(err_code, "Error (0x%X) during NDEF Select Procedure.");

            err_code = nfc_t4t_ndef_read(cc_file, ndef_files_buffs[i], TAG_TYPE_4_NDEF_FILE_SIZE);
            T4T_ERROR_HANDLE(err_code, "Error (0x%X) during NDEF Read Procedure.");
        }

        p_tlv_block++;
    }

    nfc_t4t_cc_file_printout(cc_file);

    p_tlv_block = cc_file->p_tlv_block_array;

    for (i = 0; i < cc_file->tlv_count; i++)
    {
        if ((p_tlv_block->type == NDEF_FILE_CONTROL_TLV) ||
            (p_tlv_block->value.file.p_content != NULL))
        {
            ndef_data_analyze(p_tlv_block->value.file.p_content + TAG_TYPE_4_NLEN_FIELD_SIZE,
                              p_tlv_block->value.file.len - TAG_TYPE_4_NLEN_FIELD_SIZE);
        }

        p_tlv_block++;
    }

    return NRF_SUCCESS;
}


/**
 * @brief Function for identifying Tag Platform Type.
 */
nfc_tag_type_t tag_type_identify(uint8_t sel_res)
{
    uint8_t platform_config;

    // Check if Cascade bit in SEL_RES response is cleared. Cleared bit indicates that NFCID1 complete.
    if (!IS_SET(sel_res, SEL_RES_CASCADE_BIT_NUM))
    {
        // Extract platform configuration from SEL_RES response.
        platform_config = (sel_res & SEL_RES_TAG_PLATFORM_MASK) >> SEL_RES_TAG_PLATFORM_BIT_OFFSET;
        if (platform_config < NFC_TT_NOT_SUPPORTED)
        {
            return (nfc_tag_type_t) platform_config;
        }
    }

    return NFC_TT_NOT_SUPPORTED;
}


/**
 * @brief Function for detecting a Tag, identifying its Type and reading data from it.
 *
 * This function waits for a Tag to appear in the field. When a Tag is detected, Tag Platform
 * Type (2/4) is identified and appropriate read procedure is run.
 */
ret_code_t tag_detect_and_read(void)
{
    ret_code_t     err_code;
    nfc_a_tag_info tag_info;

    // Detect a NFC-A Tag in the field and initiate a communication. This function activates
    // the NFC RF field. If a Tag is present, basic information about detected Tag is returned
    // in tag info structure.
    err_code = adafruit_pn532_nfc_a_target_init(&tag_info, TAG_DETECT_TIMEOUT);

    if (err_code != NRF_SUCCESS)
    {
        return NRF_ERROR_NOT_FOUND;
    }
    adafruit_pn532_tag_info_printout(&tag_info);

    NRF_LOG_INFO("UID JE: %d %d prvi i zadnji clan",tag_info.nfc_id[0], tag_info.nfc_id[3]);

    if( tag_info.nfc_id[0] == 0x54 && tag_info.nfc_id[1] == 0x43 && tag_info.nfc_id[2] == 0xB8 && tag_info.nfc_id[3] == 0x02 )
    {
        NRF_LOG_INFO("Uvjet ispunjen!");
        nrf_gpio_pin_set(28);
        nrf_gpio_pin_set(27);
        nrf_delay_ms(700);
        nrf_gpio_pin_clear(28);
        nrf_gpio_pin_clear(27);
    }

    nfc_tag_type_t tag_type = tag_type_identify(tag_info.sel_res);
    switch (tag_type)
    {
        case NFC_T2T:
            NRF_LOG_INFO("Type 2 Tag Platform detected. ");
            return t2t_data_read_and_analyze(&tag_info);

        case NFC_T4T:
            NRF_LOG_INFO("Type 4 Tag Platform detected. ");
            return t4t_data_read_and_analyze(&tag_info);

        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }
}


/**
 * @brief Function for waiting specified time after a Tag read operation.
 */
void after_read_delay(void)
{
    ret_code_t err_code;

    // Turn off the RF field.
    err_code = adafruit_pn532_field_off();
    APP_ERROR_CHECK(err_code);
    nrf_delay_ms(TAG_AFTER_READ_DELAY);
}

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"





/**@brief Callback function for asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num   Line number of the failing ASSERT call.
 * @param[in] file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(DEAD_BEEF, line_num, p_file_name);
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    ret_code_t err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

}

/**@brief Function for handling Queued Write Module errors.
 *
 * @details A pointer to this function will be passed to each service which may need to inform the
 *          application about an error.
 *
 * @param[in]   nrf_error   Error code containing information about what went wrong.
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**@brief Function for putting the chip into sleep mode.
 *
 * @note This function will not return.
 */
static void sleep_mode_enter(void)
{
    ret_code_t err_code;

    err_code = bsp_indication_set(BSP_INDICATE_IDLE);
    APP_ERROR_CHECK(err_code);

    // Prepare wakeup buttons.
    err_code = bsp_btn_ble_sleep_mode_prepare();
    APP_ERROR_CHECK(err_code);

    // Go to system-off mode (this function will not return; wakeup will cause a reset).
    err_code = sd_power_system_off();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for handling events from the BSP module.
 *
 * @param[in]   event   Event generated when button is pressed.
 */
static void bsp_event_handler(bsp_event_t event)
{
    ret_code_t err_code;

    switch (event)
    {
        case BSP_EVENT_SLEEP:
            sleep_mode_enter();
            break; // BSP_EVENT_SLEEP

        case BSP_EVENT_DISCONNECT:
            //err_code = sd_ble_gap_disconnect(m_conn_handle,
                                            // BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }
            break; // BSP_EVENT_DISCONNECT

        case BSP_EVENT_WHITELIST_OFF:
            //if (m_conn_handle == BLE_CONN_HANDLE_INVALID)
            //{
                //err_code = ble_advertising_restart_without_whitelist(&m_advertising);
              //  if (err_code != NRF_ERROR_INVALID_STATE)
                //{
                  //  APP_ERROR_CHECK(err_code);
                //}
            //}
            break; // BSP_EVENT_KEY_0

        default:
            break;
    }
}




/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    ret_code_t err_code;
    bsp_event_t startup_event;

    //err_code = bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, bsp_event_handler);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
}


/**@brief Function for initializing the nrf log module.
 */
static void log_init(void)
{
    ret_code_t err_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_DEFAULT_BACKENDS_INIT();
}




/**@brief Function for handling the idle state (main loop).
 *
 * @details If there is no pending log operation, then sleep until next the next event occurs.
 */
static void idle_state_handle(void)
{
    if (NRF_LOG_PROCESS() == false)
    {

        nrf_pwr_mgmt_run();
    }
}


#include "software_uart.h"
#include "rtc_external.h"

int main(void)
{
    ret_code_t     err_code;
    //log_init(); //Testing purpose

    tstr_module_init  *modules = get_modules_init();

    for (uint8_t i = 0; i < MODULE_END; i++){
        if(modules[i].is_enabled)
        {
            modules[i].init();
        }
    }

    timers_init();
    uart_tx_init();
    twi_init();

    //RTC Example
    if (mcp79402_check()) {
        // RTC - 12:34:56
        mcp79402_set_time(56, 34, 12);
        mcp79402_start_oscillator(56);

        uint8_t sec, min, hour;
        mcp79402_read_time(&sec, &min, &hour);
        while (1) {
        nrf_delay_ms(1000);
        mcp79402_read_time(&sec, &min, &hour);
        }
    } else {
        // RTC not responding
        while (1);
    }
    //RTC Example
}


/**
 * @}
 */