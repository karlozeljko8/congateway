#include "stdio.h"
#include "stdlib.h"

#include "ble_manager.h"


typedef void    (*ble_init_fn) (void);
typedef void    (*gap_init_fn) (void); 
typedef void    (*gatt_init_fn) (void);
typedef void    (*services_init_fn) (void);
typedef void    (*advertising_init_fn) (void);
typedef void    (*conn_params_init_fn) (void);
typedef void    (*peer_manager_init_fn) (void);
typedef void    (*advertising_start_fn) (bool erase_bonds);

typedef struct _tstr_ble_if {
    ble_init_fn             ble_init;
    gap_init_fn             gap_init;       
    gatt_init_fn            gatt_init;
    services_init_fn        services_init;
    advertising_init_fn     advertising_init;
    conn_params_init_fn     conn_params_init;
    peer_manager_init_fn    peer_manager_init;
    advertising_start_fn    advertising_start;

} tstr_ble_if;


extern const tstr_ble_if *pstr_ble_if;