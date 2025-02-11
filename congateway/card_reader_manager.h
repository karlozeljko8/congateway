#include "adafruit_pn532.h"
#include "nfc_t2t_parser.h"
#include "nfc_t4t_cc_file.h"
#include "nfc_t4t_hl_detection_procedures.h"
#include "nfc_ndef_msg_parser.h"

#include "nrf_log.h"

#include "app_error.h"
#include "nrf_delay.h"

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

#define MIFARE_DEFAULT_KEY  {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF} // Default MIFARE auth key
#define MIFARE_CUSTOM_KEY   {0xA1, 0x3C, 0x77, 0xC4, 0x83, 0x5B} // Custom MIFARE auth key

#define MEMORY_BLOCK    4

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

//Prototypes
ret_code_t mifare_start(void);
void after_read_delay(void);


