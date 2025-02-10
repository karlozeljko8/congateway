

#include <stdint.h>
#include <stdio.h>
#include "ble_fsm_actions.h"
//#include "ble_fsm_context.h"



typedef void (*state_handler_t) (fsm_context_t *, fsm_event_t);

void state_idle(fsm_context_t* fsm, fsm_event_t event);
void state_connected(fsm_context_t* fsm, fsm_event_t event);
void state_first_connection(fsm_context_t* fsm, fsm_event_t event);
void state_subsequent_connection(fsm_context_t* fsm, fsm_event_t event);
void state_door_open(fsm_context_t* fsm, fsm_event_t event);
void state_sleep(fsm_context_t* fsm, fsm_event_t event);



void fsm_run(fsm_context_t *fsm, fsm_event_t event);
fsm_event_t fsm_get_next_event(fsm_context_t *fsm, fsm_event_t event, fsm_state_t current_state);


