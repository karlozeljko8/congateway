
#include "peripheral_manager.h"
#include "adafruit_pn532.h"

#define RTC_TICKS_10_SEC (10 * 32768)
#define TOTAL_PINS 38 // Total number of GPIO pins in nRF52
// Od 38 - 48 visoka potrosnja, BLE radi
// Od 28 - 38 visoka potrosnja, BLE radi
// Od 18 - 28 visoka potrosnja, BLE radi
// Od 0 - 38 niska potrosnja, BLE ne radi

void rtc_init(void)
{
    nrf_drv_clock_lfclk_request(NULL);

    NRF_RTC1->PRESCALER     = 0;
    NRF_RTC1->CC[0] =       RTC_TICKS_10_SEC;

    NRF_RTC1->INTENSET  =   RTC_INTENSET_COMPARE0_Msk;
    NVIC_EnableIRQ(RTC1_IRQn);

    NRF_RTC1->TASKS_START   = 1;

}

void RTC1_IRQHandler(void)
{
    if (NRF_RTC1->EVENTS_COMPARE[0]) {
        NRF_RTC1->EVENTS_COMPARE[0] = 0;  // Clear the compare event

        // Call handler to put device to sleep
         rtc_handler(NRF_RTC_EVENT_COMPARE_0);
}
}

void rtc_handler(nrf_rtc_event_t event){
    if (event == NRF_RTC_EVENT_COMPARE_0){
        NRF_RTC1->TASKS_CLEAR   =1;
        //adafruit_pn532_power_down();
        //nrf_pwr_mgmt_shutdown(0);
    }
}

void gpio_config(void){
    configure_unused_pins();
    configure_pn532_pins();
}

void configure_unused_pins(void) {
    for (uint32_t pin = 8; pin < 32; pin++) {
        // Configure pin as input
        nrf_gpio_cfg_input(pin, NRF_GPIO_PIN_PULLUP);
    }
}

void configure_pn532_pins(void){
    nrf_gpio_cfg_input(PN532_IRQ, NRF_GPIO_PIN_NOPULL);
    // The reset pin in the AdaFruit NFC Shield is actually the PN532 reset indicator pin (RSTOUT_N)
    // and cannot be used to perform reset of the chip. (RSTPD_N pin, see AdaFruit NFC Shield
    // schematics).
    nrf_gpio_cfg_input(PN532_RESET, NRF_GPIO_PIN_NOPULL); // Vjerojatno se ne koristi
}


//static const nrf_drv_twi_t m_twi_master = NRF_DRV_TWI_INSTANCE(PN532_CONFIG_TWI_INSTANCE);

void i2c_init(void){
    const nrf_drv_twi_t *m_twi_master = get_m_twi_master();

    nrf_drv_twi_config_t twi_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    twi_config.scl = I2C_SCL;
    twi_config.sda = I2C_SDA;

    ret_code_t ret = nrf_drv_twi_init(m_twi_master, &twi_config, NULL, NULL);
    if (ret != NRF_SUCCESS)
    {
        return ret;
    }

    nrf_drv_twi_enable(m_twi_master);

    return NRF_SUCCESS;
}