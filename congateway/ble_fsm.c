

#include "ble_fsm.h"
#include "ble.h"

#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "nrf_log.h"

state_handler_t state_handlers [] = {
    state_idle,
    state_connected,
    state_first_connection,
    state_subsequent_connection,
    state_door_open,
    state_sleep
};

void fsm_run(fsm_context_t *fsm, fsm_event_t event){
    fsm_event_t current_event = event;
    fsm_state_t current_state;

    do{
        NRF_LOG_INFO("fsm run with event: %d in state: %d", current_event, fsm->current_state);
        current_state = fsm->current_state;
        state_handlers[fsm->current_state](fsm, current_event);

        current_event = fsm_get_next_event(fsm, current_event, current_state);

    }while(current_event != EVENT_NO_EVENT);
}

void fsm_add_event(fsm_context_t *fsm, fsm_event_t event){
    fsm->event_queue[fsm->queue_tail] = event;

    fsm->queue_tail = (fsm->queue_tail + 1) % 10 ;

}

fsm_event_t fsm_get_next_event(fsm_context_t *fsm, fsm_event_t event, fsm_state_t current_state){
    fsm_event_t next_event = event;

    if((fsm->queue_head == fsm->queue_tail) && (current_state == fsm->current_state)){
        next_event = EVENT_NO_EVENT;
        return next_event;
    }

    if((fsm->queue_head == fsm->queue_tail) && (current_state != fsm->current_state)){
        return next_event;
    }    

    next_event = fsm->event_queue[fsm->queue_head];
    fsm->queue_head = (fsm->queue_head + 1) % 10;

    return next_event;
}

void state_idle(fsm_context_t *fsm, fsm_event_t event){
    switch(event)
    {
        case EVENT_DATA_RCVD:
            fsm->current_state = STATE_CONNECTED;
            break;
    }
}

void state_connected(fsm_context_t *fsm, fsm_event_t event){
    fsm_event_t next_event = EVENT_NO_EVENT;

    switch(event)
    {
        case EVENT_DATA_RCVD:

            fsm_add_event(fsm, EVENT_CHECK_PASSWORD); // will be return from action
            break;
        case EVENT_CHECK_PASSWORD:
            next_event = action_check_password(fsm, event);
            fsm_add_event(fsm, next_event);
            break;
        case EVENT_FIRST_CONNECTION:
            fsm->current_state = STATE_FIRST_CONNECTION;
            break;
        case EVENT_SUBSEQUENT_CONNECTION:
            fsm->current_state = STATE_SUBSEQUENT_CONNECTION;
            break;
        case EVENT_PASSWORD_INVALID:
            fsm->current_state = STATE_SLEEP;
            break;
        default:
            NRF_LOG_INFO("Not expected event occurs!");
    }
}

void state_first_connection(fsm_context_t *fsm, fsm_event_t event)
{
    fsm_event_t next_event = EVENT_NO_EVENT;

    switch(event)
    {
        case EVENT_FIRST_CONNECTION:
            next_event = action_check_serial_number(fsm, event);

            fsm_add_event(fsm, next_event); // will be return from action
            // ili fsm_add_event(fsm, EVENT_SERIAL_INVALID);
            break;
        case EVENT_SERIAL_VALID:
            next_event = action_send_unique_password(fsm, event);
            fsm_add_event(fsm, next_event);
            
            fsm->current_state = STATE_DOOR_OPEN;
            break;
        case EVENT_SERIAL_INVALID:

            fsm->current_state = STATE_SLEEP;
            break;
        case EVENT_GO_TO_SLEEP:
            fsm->current_state = STATE_SLEEP;
            break;
    default:
        fsm_add_event(fsm, EVENT_NO_EVENT); // think about it
    }
}

void state_subsequent_connection(fsm_context_t *fsm, fsm_event_t event){
    switch(event)
    {
        case EVENT_SUBSEQUENT_CONNECTION:
            

            fsm_add_event(fsm, EVENT_PASSWORD_VALID); // will be return from action
            // ili fsm_add_event(fsm, EVENT_SERIAL_INVALID);
            break;
        case EVENT_PASSWORD_VALID:
            //action_send_unique_pass
            fsm->current_state = STATE_DOOR_OPEN;
            break;
        case EVENT_SERIAL_INVALID:

            fsm->current_state = STATE_SLEEP;
            break;
        default:
            fsm_add_event(fsm, EVENT_NO_EVENT); // think about it
    }
}

void state_door_open(fsm_context_t *fsm, fsm_event_t event){
    switch(event)
    {
        case EVENT_PASSWORD_VALID:   
            //action_door_open
            NRF_LOG_INFO("Uvjet ispunjen!");
            nrf_gpio_pin_set(28);
            nrf_gpio_pin_set(27);
            nrf_delay_ms(1500);
            nrf_gpio_pin_clear(28);
            nrf_gpio_pin_clear(27);
            fsm_add_event(fsm, EVENT_WRITE_USR_INFO); // will be return from action
            break;
        case EVENT_SERIAL_VALID:
            //action_open_door
            fsm_add_event(fsm, EVENT_WRITE_USR_INFO); // will be return from action
            fsm->current_state = STATE_DOOR_OPEN;
            break;
        case EVENT_WRITE_USR_INFO:
            //TO DO: create action that write serial num and maybe some timestamp into buffer
            fsm_add_event(fsm, EVENT_GO_TO_SLEEP);
            fsm->current_state = STATE_SLEEP;
            break;
        default:
            fsm_add_event(fsm, EVENT_NO_EVENT); // think about it
    }
}

void state_sleep(fsm_context_t *fsm, fsm_event_t event){
    switch(event)
    {
        case EVENT_GO_TO_SLEEP:   
            //action_go_to_sleep
            fsm_add_event(fsm, EVENT_NO_EVENT); // will be return from action
            break;
        case EVENT_SERIAL_INVALID:
            //action_go_to_sleep
            fsm_add_event(fsm, EVENT_NO_EVENT); // will be return from action
            break;
        case EVENT_PASSWORD_INVALID:
            //action_go_to_sleep
            fsm_add_event(fsm, EVENT_NO_EVENT); // will be return from action
            break;
        default:
            fsm_add_event(fsm, EVENT_NO_EVENT); // think about it
    }
}