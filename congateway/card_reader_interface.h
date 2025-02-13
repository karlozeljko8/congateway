#include "stdio.h"
#include "stdlib.h"
#include "sdk_errors.h"

#include "card_reader_manager.h"


typedef ret_code_t (*pn532_init_fn)(bool force);
typedef ret_code_t (*mifare_process_start_fn)(void);

typedef struct _tstr_card_reader_if {
    pn532_init_fn           pn532_init;
    mifare_process_start_fn mifare_process_start;

}tstr_card_reader_if;


extern const tstr_card_reader_if *pstr_card_reader_if;
