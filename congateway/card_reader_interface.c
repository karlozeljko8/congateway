#include "card_reader_interface.h"


static const tstr_card_reader_if str_card_reader_if = {
    .pn532_init             = adafruit_pn532_init,
    .mifare_process_start   = mifare_start
};

const tstr_card_reader_if *pstr_card_reader_if = &str_card_reader_if;