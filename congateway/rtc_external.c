#include "rtc_external.h"

#define TWI_INSTANCE_ID 0
static const nrf_drv_twi_t twi_instance = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

// Provjeriti je li potrebno kada se upogoni sa PN532 koji isto koristi I2C
void twi_init(void) {
    ret_code_t err_code;

    const nrf_drv_twi_config_t twi_config = {
        .scl                = 18,
        .sda                = 17,
        .frequency          = NRF_TWI_FREQ_100K,
        .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
        .clear_bus_init     = false
    };

    err_code = nrf_drv_twi_init(&twi_instance, &twi_config, NULL, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&twi_instance);
}

bool mcp79402_check(void) {
    uint8_t reg = RTC_SEC_REG;
    uint8_t response;

    ret_code_t err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, &reg, 1, true);
    if (err_code != NRF_SUCCESS) {
        return false;
    }

    err_code = nrf_drv_twi_rx(&twi_instance, MCP79402_I2C_ADDRESS, &response, 1);
    return (err_code == NRF_SUCCESS);
}

void mcp79402_set_time(uint8_t sec, uint8_t min, uint8_t hour) {
    uint8_t data[2];

    data[0] = RTC_SEC_REG;
    data[1] = sec & 0x7F;
    ret_code_t err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, data, sizeof(data), false);
    APP_ERROR_CHECK(err_code);
    
    data[0] = RTC_MIN_REG;
    data[1] = min & 0x7F;  
    err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, data, sizeof(data), false);
    APP_ERROR_CHECK(err_code);

    data[0] = RTC_HOUR_REG;
    data[1] = hour & 0x3F; 

     err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, data, sizeof(data), false);
    APP_ERROR_CHECK(err_code);
}

void mcp79402_start_oscillator(uint8_t sec) {
    uint8_t data[2];

    data[0] = RTC_SEC_REG;           
    data[1] = sec | 0x80; //Dignuti 7 bit u jedinicu za startanje oscilatora          

    ret_code_t err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, data, sizeof(data), false);
    APP_ERROR_CHECK(err_code);
}

void mcp79402_read_time(uint8_t *sec, uint8_t *min, uint8_t *hour) {
    uint8_t reg = RTC_SEC_REG;
    uint8_t data[3];

    ret_code_t err_code = nrf_drv_twi_tx(&twi_instance, MCP79402_I2C_ADDRESS, &reg, 1, true);
    APP_ERROR_CHECK(err_code);

    // Read seconds, minutes, and hours
    err_code = nrf_drv_twi_rx(&twi_instance, MCP79402_I2C_ADDRESS, data, sizeof(data));
    APP_ERROR_CHECK(err_code);

    *sec = data[0] & 0x7F;   // Mask to get seconds
    *min = data[1] & 0x7F;   // Mask to get minutes
    *hour = data[2] & 0x3F;  // Mask to get hours
}