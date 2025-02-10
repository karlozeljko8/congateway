#include "nfc_manager.h"
#include "nrf52.h"
#include "nrf52_bitfields.h"
#include "stdbool.h"


void nfc_wakeup_config(void) {
    // Configure the NFC pins in NFC mode
    //nrf_gpio_cfg_default(9);  // NFC1
    //nrf_gpio_cfg_default(10); // NFC2
    
    // Enable NFC
    NRF_NFCT->TASKS_SENSE = 1;  // Start sensing for NFC field

    // Enable wake-up on NFC detection
    NRF_NFCT->INTENSET = NFCT_INTENSET_FIELDDETECTED_Msk;  // Enable FIELDDETECTED interrupt
    NVIC_EnableIRQ(NFCT_IRQn);                             // Enable NFC interrupt in NVIC
}

void NFCT_IRQHandler(void) {
    if (NRF_NFCT->EVENTS_FIELDDETECTED) {
        // Clear the event
        NRF_NFCT->EVENTS_FIELDDETECTED = 0;

        // Reset the device to simulate wake-up behavior
        NVIC_SystemReset();
    }
}