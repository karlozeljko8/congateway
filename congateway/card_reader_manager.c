#include "card_reader_manager.h"


/**
 * @brief Possible Tag Types.
 */
typedef enum
{
    NFC_T2T = 0x00,      ///< Type 2 Tag Platform.
    NFC_T4T = 0x01,      ///< Type 4A Tag Platform.
    NFC_TT_NOT_SUPPORTED ///< Tag Type not supported.
} nfc_tag_type_t;

typedef struct {
    uint8_t default_key[6]; // Default Auth key 
    uint8_t custom_key[6];  // Custom Auth key
    uint8_t memory_block;   // Memory block for password store   
    uint8_t sector;         // Sector of memory block(different for 1kB and 4kB Mifare cards)         
    uint8_t uid[7];         // Buffer for UID        
    uint8_t uid_len;
    uint8_t block_data[16]; // Buffer for block data
}tstr_mifare_data;

typedef enum _tenu_card_read_state  {
    INIT,
    AUTH_WITH_DEFAULT_KEY,
    AUTH_WITH_CUSTOM_KEY,
    CONFIG_CUSTOM_KEY,
    READ_MEMORY_BLOCK,
    WRITE_TO_MEMORY_BLOCK
}tenu_card_read_state;

tstr_mifare_data mifare_data = {
    .default_key    = MIFARE_DEFAULT_KEY,
    .custom_key     = MIFARE_CUSTOM_KEY,
    .memory_block   = MEMORY_BLOCK,
    .sector         = SECTOR,
    .uid            = {0},
    .uid_len        = 0,
    .block_data     = {0}
};

tenu_card_read_state card_read_state = INIT;
tenu_card_read_state init_next_state = AUTH_WITH_DEFAULT_KEY; // Next state after initialization

tstr_mifare_data *get_mifare_data(void){

    return &mifare_data;
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

void print_card_uid(tstr_mifare_data *mifare_data){
    NRF_LOG_INFO("Card detected with UID:");

    for (uint8_t i = 0; i < mifare_data->uid_len; i++) {
        NRF_LOG_INFO("%02X ", mifare_data->uid[i]);
    }
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
    //uart_tx_buffer(tag_info.nfc_id,4, 200);
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

// Authentication
ret_code_t mifare_authenticate(uint8_t block_number, uint8_t *key, uint8_t *uid, uint8_t uid_len) {
    uint8_t auth_cmd[12];
    auth_cmd[0] = MIFARE_CMD_AUTH_A; // Use MIFARE_CMD_AUTH_B for KeyB
    auth_cmd[1] = block_number;     // Block to authenticate
    memcpy(&auth_cmd[2], key, 6);   // Authentication key (6 bytes)
    memcpy(&auth_cmd[8], uid, uid_len); // Card UID

    uint8_t response_len = 0;
    return adafruit_pn532_in_data_exchange(auth_cmd, sizeof(auth_cmd), NULL, &response_len);
}

// Read Block
ret_code_t mifare_read_block(uint8_t block_number, uint8_t *data) {
    uint8_t read_cmd[2];
    read_cmd[0] = MIFARE_CMD_READ;
    read_cmd[1] = block_number;

    uint8_t response_len = 16; // Block size is 16 bytes
    return adafruit_pn532_in_data_exchange(read_cmd, sizeof(read_cmd), data, &response_len);
}

// Write Block
ret_code_t mifare_write_block(uint8_t memory_block, uint8_t *data) {
    uint8_t write_cmd[18];
    write_cmd[0] = MIFARE_CMD_WRITE;
    write_cmd[1] = memory_block;
    memcpy(&write_cmd[2], data, 16); // Data to write

    uint8_t response_len = 0;
    return adafruit_pn532_in_data_exchange(write_cmd, sizeof(write_cmd), NULL, &response_len);
}


uint8_t test_uid[4] =   {0x43, 0x11, 0x1D, 0xD8};

ret_code_t memory_block_authenticate(tstr_mifare_data *mifare_data, uint8_t *key){
    ret_code_t err_code;
    //  Authenticate defined block with appropriate key 
    err_code = mifare_authenticate(mifare_data->memory_block, key, mifare_data->uid, mifare_data->uid_len);

    if (err_code != NRF_SUCCESS) {
        NRF_LOG_INFO("Authentication failed.");
        return err_code;
    }

    NRF_LOG_INFO("Authentication successful.");
    return err_code;

    // Step 3: Read block 4
    //err_code = mifare_read_block(memory_block, block_data);
    //if (err_code == NRF_SUCCESS) {
    //    NRF_LOG_INFO("Block 4 Data:");
    //    for (uint8_t i = 0; i < 16; i++) {
    //        NRF_LOG_INFO("%02X ", block_data[i]);
    //    }
    //    return err_code;
    //} else {
    //    NRF_LOG_INFO("Failed to read block.");
    //    return err_code;
    //} 
}

ret_code_t configure_custom_key(tstr_mifare_data *mifare_data){
    uint8_t sector_trailer_block    = (mifare_data->sector * 4) + 3; // trailer block - zadnji block u sektoru
    uint8_t sector_trailer_data[16]      = {0};

        // 1. Authenticate with the default key
    ret_code_t err_code = mifare_authenticate(sector_trailer_block, mifare_data->default_key,
                                                mifare_data->uid, mifare_data->uid_len);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Authentication failed.");
        return err_code;
    }  

    // 2. Prepare the sector trailer data
    memcpy(sector_trailer_data, mifare_data->custom_key, 6);           // Key A (new custom key)
    sector_trailer_data[6] = 0xFF;                     // Access bits (default values)
    sector_trailer_data[7] = 0x07;                     // Access bits
    sector_trailer_data[8] = 0x80;                     // Access bits
    memset(&sector_trailer_data[9], 0x00, 1);          // General purpose byte
    memcpy(&sector_trailer_data[10], mifare_data->default_key, 6);  // Key B - ostavljen kao defaultni

    // 3. Write the new sector trailer
    err_code = mifare_write_block(sector_trailer_block, sector_trailer_data);
    if (err_code != NRF_SUCCESS) {
        NRF_LOG_ERROR("Failed to write sector trailer.");
        return err_code;
    }

    NRF_LOG_INFO("Custom key configured successfully.");
    return NRF_SUCCESS;

}

nfc_a_tag_info tag_info;



/// @brief When mifare card is detected, defined memory block will
//         be checked for password. If password is okay, door will
//         be opened. If values are default, UID will be checked if
//         is on the list. If it is, password will be allocated to card
/// @return 
ret_code_t mifare_start(void) {
    ret_code_t err_code;



    // TEST DATA
    uint8_t new_data[16] = {
        0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };
    // TEST DATA

    tstr_mifare_data *ptr_mifare_data = get_mifare_data();

    switch(card_read_state){
        case INIT:
            //  Card detection
            err_code = adafruit_pn532_nfc_a_target_init(&tag_info, 1000);
            
            if (err_code == NRF_SUCCESS) {
                // Extract UID
                memcpy(ptr_mifare_data->uid, tag_info.nfc_id, tag_info.nfc_id_len);
                ptr_mifare_data->uid_len = tag_info.nfc_id_len;

                print_card_uid(ptr_mifare_data);
                card_read_state = init_next_state; 
            }else{
                NRF_LOG_INFO("No tag detected.");
            }
        break;
        case AUTH_WITH_DEFAULT_KEY:
            err_code = memory_block_authenticate(ptr_mifare_data, ptr_mifare_data->default_key);
            if (err_code == NRF_SUCCESS){
                card_read_state = CONFIG_CUSTOM_KEY;
            }else{
                card_read_state = INIT; // Reinit
                init_next_state = AUTH_WITH_CUSTOM_KEY; // After reinit - go to AUTH_WITH_CUSTOM_KEY
            }
        break;
        case AUTH_WITH_CUSTOM_KEY:
            NRF_LOG_INFO("Auth with Custom key!");
            err_code = memory_block_authenticate(ptr_mifare_data, ptr_mifare_data->custom_key);
            if (err_code == NRF_SUCCESS){
                card_read_state = WRITE_TO_MEMORY_BLOCK;
            }else{
                card_read_state = INIT; // Reinit
                init_next_state = AUTH_WITH_DEFAULT_KEY; // After reinit - go to AUTH_WITH_DEFAULT_KEY
            }
    
        break;
        case CONFIG_CUSTOM_KEY:
            NRF_LOG_INFO("Config custom key!");
            configure_custom_key(ptr_mifare_data);
            if (err_code == NRF_SUCCESS){
                card_read_state = AUTH_WITH_CUSTOM_KEY;
            }else{
                card_read_state = INIT; // Reinit
                init_next_state = AUTH_WITH_DEFAULT_KEY; // After reinit - go to AUTH_WITH_DEFAULT_KEY
            }
        break;
        case WRITE_TO_MEMORY_BLOCK:
            NRF_LOG_INFO("Write to memory block!");
            err_code = mifare_write_block(ptr_mifare_data->memory_block, new_data);
            if (err_code == NRF_SUCCESS) {
                NRF_LOG_INFO("Block %d written successfully in sector %d.", ptr_mifare_data->memory_block, ptr_mifare_data->sector);
            }else {
                NRF_LOG_INFO("Failed to write block 4.");
            }
            card_read_state = INIT;
            init_next_state = AUTH_WITH_DEFAULT_KEY; // After reinit - go to AUTH_WITH_DEFAULT_KEY
        break;
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