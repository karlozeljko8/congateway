
#include <stdint.h>
#include <stdio.h>
#include "ble.h"



#define PASSWORD_LEN 0x04
#define SERIAL_NUMBER_LEN 0x06


typedef struct _tstr_received_data
{
    uint8_t serial_number[SERIAL_NUMBER_LEN];
    uint8_t password[PASSWORD_LEN];
}tstr_received_data;

// Forward declarations to avoid circular dependencies
typedef struct ble_cus_s ble_cus_t;

// FSM states
typedef enum {
    STATE_IDLE,
    STATE_CONNECTED,
    STATE_FIRST_CONNECTION,
    STATE_SUBSEQUENT_CONNECTION,
    STATE_DOOR_OPEN,
    STATE_SLEEP
} fsm_state_t;

typedef enum{
    EVENT_DATA_RCVD,
    EVENT_CHECK_PASSWORD,
    EVENT_FIRST_CONNECTION,
    EVENT_SUBSEQUENT_CONNECTION,
    EVENT_PASSWORD_VALID,
    EVENT_PASSWORD_INVALID,
    EVENT_SEND_UNIQUE_PASSWORD,
    EVENT_SERIAL_VALID,
    EVENT_SERIAL_INVALID,
    EVENT_WRITE_USR_INFO,
    EVENT_GO_TO_SLEEP,
    EVENT_NO_EVENT
} fsm_event_t;

typedef struct {
    fsm_state_t current_state;
    fsm_event_t current_event;
    ble_cus_t *p_cus;
    ble_evt_t const *p_ble_evt;
    tstr_received_data received_data;
    fsm_event_t event_queue[10];
    uint8_t queue_head;
    uint8_t queue_tail;
} fsm_context_t;