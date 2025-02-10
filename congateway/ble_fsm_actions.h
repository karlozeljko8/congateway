

#include "ble_fsm_context.h"
#include <string.h>
#include "sdk_errors.h"


static ret_code_t ble_send_data(fsm_context_t *fsm, ble_gatts_value_t *gatts_value);

fsm_event_t action_send_unique_password(fsm_context_t *fsm, fsm_event_t event);
fsm_event_t action_check_password(fsm_context_t *fsm, fsm_event_t event);
fsm_event_t action_check_serial_number(fsm_context_t *fsm, fsm_event_t event);
