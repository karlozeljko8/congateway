#include "nfc_interface.h"

static const tstr_nfc_if str_nfc_if = {
    .nfc_wakeup_config  = nfc_wakeup_config

};

const tstr_nfc_if *pstr_nfc_if = &str_nfc_if;