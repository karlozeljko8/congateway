#include "nfc_manager.h"

typedef void (*nfc_wakeup_config_fn) (void);


typedef struct _tstr_nfc_if {
    nfc_wakeup_config_fn    nfc_wakeup_config;

}tstr_nfc_if;

extern const tstr_nfc_if *pstr_nfc_if;