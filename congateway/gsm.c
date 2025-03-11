#include "gsm.h"
#include "nrf_gpio.h"

#include "app_uart.h"
#include "nrf_uart.h"
#include "software_uart.h"
#include "nrf_strerror.h"
#include <string.h>

#define TX 11
#define RX 12

void gsm_init(void){
    nrf_gpio_cfg_output(POWER_PIN);
}

void gsm_power_enable(void){
    nrf_gpio_pin_set(POWER_PIN);
}

void gsm_power_disable(void){
    nrf_gpio_pin_clear(POWER_PIN);
}

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}
bool cmd_sent = false;
bool modem_respond_rcvd = false;

#include "nrf_drv_uart.h"

#define RX_BUFFER_SIZE 512
static char rx_buffer[RX_BUFFER_SIZE]; // Buffer for receiving 1 byte
uint16_t rx_index;
char rx_byte;

static const nrf_drv_uart_t m_uart = NRF_DRV_UART_INSTANCE(0);

static const nrf_drv_uart_config_t m_uart_config = {
    .pseltxd            = TX,           // Replace TX_PIN_NUMBER with the pin number for TX
    .pselrxd            = RX,           // Replace RX_PIN_NUMBER with the pin number for RX
    .pselcts            = NRF_UART_PSEL_DISCONNECTED, // Disable CTS
    .pselrts            = NRF_UART_PSEL_DISCONNECTED, // Disable RTS
    .p_context          = NULL,                    // Context for callbacks, if needed
    .hwfc               = NRF_UART_HWFC_DISABLED,  // No hardware flow control
    .parity             = NRF_UART_PARITY_EXCLUDED,// No parity
    .baudrate           = NRF_UART_BAUDRATE_115200,// Set baud rate to 115200
    .interrupt_priority = APP_IRQ_PRIORITY_LOWEST, // Priority of the UART interrupt
};

void uart_event_handler(nrf_drv_uart_event_t * p_event, void * p_context)
{
    switch (p_event->type)
    {
        case NRF_DRV_UART_EVT_RX_DONE:
    
         break;

        case NRF_DRV_UART_EVT_TX_DONE:
            
            // Data transmitted; handle if necessary
            break;

        case NRF_DRV_UART_EVT_ERROR:
            // Handle UART errors
            break;

        default:
            break;
    }
}
void uart_send(uint8_t * data)
{
    ret_code_t err_code;
    size_t length = strlen(data);
    // Send data
    err_code = nrf_drv_uart_tx(&m_uart, data, length);
    uart_log("uart send: %s \r\n", data);
    if (err_code != NRF_SUCCESS)
    {
        // Handle error
    }
    cmd_sent = true;
}

char uart_read_char(void) {
    while (!(NRF_UART0->EVENTS_RXDRDY)) {
        // Cekaj dok ne primis bajt
    }
    NRF_UART0->EVENTS_RXDRDY = 0;
    return NRF_UART0->RXD;
}
void receive_buffer(){
    uint16_t timeout = 0;
    char byte;

    if (NRF_UART0->EVENTS_RXDRDY != 0 ){
        rx_index = 0;
        memset(rx_buffer, 0, sizeof(rx_buffer));
        while(timeout < 10000){
            if (NRF_UART0->EVENTS_RXDRDY != 0) {
                if (rx_index < RX_BUFFER_SIZE - 1) {
                    byte = uart_read_char();
                    rx_buffer[rx_index++] = byte;
                } else {
                    break;
                }
                timeout = 0;
            } else {
                timeout++;
            }
        }
        modem_respond_rcvd = true;
        cmd_sent           = false;
    }
    uart_log("buffer: %s ", rx_buffer);
    uart_log("buffer len: %d\r\n",rx_index);
    if((strstr((const char*)rx_buffer, "OK"))){
        uart_log("+\r\n");
    }

}
void uart_init(void){
    ret_code_t err_code;
    nrf_drv_uart_uninit(&m_uart);
    err_code = nrf_drv_uart_init(&m_uart, &m_uart_config, uart_event_handler);
    if (err_code != NRF_SUCCESS)
    {
        // Handle error
        return;
    }
    // Enable RX
    nrf_drv_uart_rx_enable(&m_uart);
    
    nrf_drv_uart_rx(&m_uart, &rx_byte, 1);
}

typedef enum {
    GSM_RESET          = 0,
    GSM_ON             = 1,
    GSM_AUTOBAUD_INIT  = 2,	   //saljem A i cekam odgovor A
    GSM_AT_INIT		= 3,	   //saljem AT+CPIN? cekam OK , dali je pin ok
    GSM_AT_CSQ		    = 4,	   //saljem AT+CSQ   ,+CSQ:20,0 OK signal qualiti report
    GSM_AT_CREG		= 5,	   //saljem AT+CREG? ,+CREG:0,1 OK log na home network
    GSM_AT_MODE        = 6,
    GSM_AT_CGATT	    = 7,	  //saljem AT+CGATT?,+CGATT:1 OK gprs service on?
    GSM_AT_SHUT        = 8, 	  //saljem AT+CIPSHUT	 cekam OK brisem aktualne apn postavke
    GSM_APN			= 9,	  //saljem AT+CSTT="APN","user","pass"	cekam OK
    GSM_APN_CONT       = 10,	  //saljem AT+CGDCONT=1,"APN" cekam OK
    GSM_CIIR           = 11,	  //saljem AT+CIICR	gprs Up? cekam OK
    GSM_CIFSR          = 12,     //saljem AT+CIFSR local ip cekam ip adesu nema oK
    GSM_CONNECT        = 13,	  //saljem AT+CIPSTART="TCP","ip.ip.ip.ip","port" cekam CONNECT OK ako ne ma connect ponovim nakon jedne minute 
    GSM_SET_SEND       = 14,	  //saljem AT+CIPSEND i cekam karakter '>' nakon toga mogu upisivati u buffer
    GSM_SEND_HALLO     = 15,	  //saljem ID
    GSM_FILL_DATA      = 16,	  //upisujme u gprs buffer podatke sa jennica 
    GSM_SEND           = 17,	  //saljem 0X1A  i cekam SEND OK
    GSM_DISCONNECT	    = 18,	  //saljem AT+CIPCLOSE cekam CLOSE OK 
    GSM_STOP           = 19,     //mirujem dok ne primi drugu komandu 
    GSM_WAIT_CONNECT   = 254,    //mirujem dok ne primim odgovor
    GSM_IDLE           = 255,    //mirujem dok ne primim odgovor
      
}gsm_states_t;
#define GPRS_TIMEOUT_MIN  5 //ako ne primim nikakvu informaciju sa servera tada aktiviram time out min
#define UART_BUFF_SIZE 256

//static uint8_t rx_buffer[UART_BUFF_SIZE];
//static uint8_t tx_buffer[UART_BUFF_SIZE];
static gsm_states_t  current_state = GSM_AT_INIT;

void set_gprs_state(gsm_states_t next_state)
{
    current_state = next_state;
}

// FTP Example
const char *ftp_server = "ftp.example.com";
const char *ftp_user = "username";
const char *ftp_pass = "password";
const char *file_path = "upload/test.txt";
const char *file_data = "Hello, FTP!";


void gprs_command_handle(void)
{
    if(!cmd_sent)
    {
        switch (current_state)
        {
            case GSM_AUTOBAUD_INIT: uart_send((uint8_t *)"A\r\n");          break;
            case GSM_AT_INIT:       uart_send((uint8_t *)"AT+CPIN?\r\n");   break;  // SIM Card status
            case GSM_AT_CSQ :       uart_send((uint8_t *)"AT+CSQ\r\n");     break;  // Signal quality
            case GSM_AT_CREG:       uart_send((uint8_t *)"AT+CREG?\r\n");   break;  // Register on home network
        }
    }
}

void gprs_data_handle(void)
{
    if(modem_respond_rcvd)
    {
        modem_respond_rcvd = false;

        switch (current_state)
        {
            case GSM_AUTOBAUD_INIT:  
                if(strstr ((const char*)rx_buffer, "A"))
                {
                    set_gprs_state(GSM_AT_INIT);
                }
                break;
            case GSM_AT_INIT:  

                if((strstr ((const char*)rx_buffer, "READY")) && (strstr ((const char*)rx_buffer, "OK")))
                {
                    set_gprs_state(GSM_AT_CSQ);
                }else
                if( strstr ((const char*)rx_buffer, "ERROR")) 
                {
                    set_gprs_state(GSM_AUTOBAUD_INIT);
                }
                break;  //SIM ready
            case GSM_AT_CSQ :

                if( strstr ((const char*)rx_buffer, "OK"))
                {
                    set_gprs_state(GSM_AT_CREG);
                } else
                if( strstr ((const char*)rx_buffer, "ERROR")) 
                {
                    //set_gprs_state(GSM_AUTOBAUD_INIT); TO DO 
                }
                break;  //Signal quality
            case GSM_AT_CREG:
                if(( strstr ((const char*)rx_buffer, "+CREG: 0,1")) && (strstr ((const char*)rx_buffer, "OK")))
                {
                    set_gprs_state(GSM_AT_MODE);
                } else
                if( strstr ((const char*)rx_buffer, "ERROR")) 
                {
                    //set_gprs_state(GSM_AUTOBAUD_INIT); TO DO
                }
                break;  //register on home network
        }
    }
}


//unsigned char gprs_command_handle(void)
//{
//
//  unsigned char data_out[15];
//  
//  switch (current_state)
//  {
//     // case GSM_RESET:   if(bit_get(FIO3PIN,LONGBIT(GSM_STAT)))
//	 //                    {
//	 //                     	bit_set(FIO3PIN,LONGBIT(GSM_PWR));
//	 //                     	while (bit_get(FIO3PIN,LONGBIT(GSM_STAT)))
//	//					  	{
//	//					  	}
//	//						;bit_clear(FIO3PIN,LONGBIT(GSM_PWR));
//	//						
//	//					 };
//	//					 set_gprs_status(GSM_IDLE);
//	//					break;
//	  case GSM_ON:      set_gprs_status(GSM_AUTOBAUD_INIT);break;
//	  case GSM_AUTOBAUD_INIT:Send_UART2((unsigned char*)"A");break;
//	  case GSM_AT_INIT: Send_UART2((unsigned char*)"AT+CPIN?\r\n");break;//sim ready
//	  case GSM_AT_CSQ : Send_UART2((unsigned char*)"AT+CSQ\r\n");break;  //quolity of service
//	  case GSM_AT_CREG: Send_UART2((unsigned char*)"AT+CREG?\r\n");break;                          //register on home network
//	  case GSM_AT_MODE: Send_UART2((unsigned char*)"AT+CIPMODE=1\r\n");break;					   // set transparent mode
//	  case GSM_AT_CGATT:Send_UART2((unsigned char*)"AT+CGATT?\r\n");break;							//gprs service on 
//	  case GSM_AT_SHUT: Send_UART2((unsigned char*)"AT+CIPSHUT\r\n");break;							//gprs clear old apn setup 
//	  case GSM_APN:     Send_UART2((unsigned char*)"AT+CSTT=");											//AT+CSTT="data.vip.hr","38591","38591"\r\n
//	                    // Provjeriti sendchar2('"');Send_UART2((unsigned char*)routers.gprs.gprs_apn );sendchar2('"');sendchar2(',');
//				        // Provjeriti sendchar2('"');Send_UART2((unsigned char*)routers.gprs.gprs_user);sendchar2('"');sendchar2(',');
//				        // Provjeriti sendchar2('"');Send_UART2((unsigned char*)routers.gprs.gprs_pass);sendchar2('"');Send_UART2((unsigned char*)"\r\n");                                                            
//	                    break;
//	  case GSM_APN_CONT:
//	  			        Send_UART2((unsigned char*)"AT+CGDCONT=1,");	//AT+CGDCONT=1,"IP","APN"
//				        sendchar2('"');Send_UART2((unsigned char*)"IP");sendchar2('"');sendchar2(',');
//				        // Provjeriti!! sendchar2('"');Send_UART2((unsigned char*)routers.gprs.gprs_apn);sendchar2('"');
//				        Send_UART2((unsigned char*)"\r\n");break;
//	  case GSM_CIIR:
//	                    Send_UART2((unsigned char*)"AT+CIICR\r\n");set_gprs_status(GSM_IDLE);break;
//	  case GSM_CIFSR:
//	  					Send_UART2((unsigned char*)"AT+CIFSR\r\n");break;
//						
//	  case GSM_CONNECT:	//AT+CIPSTART="TCP","192.241.160.216","8888"
//						
//	  					Send_UART2((unsigned char*)"AT+CIPSTART=");sendchar2('"');Send_UART2((unsigned char*)"TCP");sendchar2('"');sendchar2(',');sendchar2('"');
//						// PROVJERITI !!sprintf((char*)data_out,"%d.%d.%d.%d",(int)routers.gprs.host_ip[0],(int)routers.gprs.host_ip[1],(int)routers.gprs.host_ip[2],(int)routers.gprs.host_ip[3]);
//						Send_UART2(data_out);sendchar2('"');sendchar2(',');sendchar2('"');
//						// PROVJERITI !!sprintf((char*)data_out,"%d",(int)routers.gprs.host_port);
//						Send_UART2(data_out);sendchar2('"');Send_UART2((unsigned char*)"\r\n");
//						set_gprs_status(GSM_WAIT_CONNECT);break;
//	  //case GSM_SET_SEND:Send_UART2("AT+CIPSEND\r\n");set_gprs_status(GSM_IDLE);break;
//	  case GSM_SEND_HALLO:
//	                    //PROVJERITI Send_UART2((unsigned char*)">HALLO ID:");Send_UART2((unsigned char*)routers.client_name),sendchar2(0X1A);
//						set_gprs_status(GSM_FILL_DATA);break;
//	  case GSM_FILL_DATA:;
//	                     break;
//	  //case GSM_SEND:    sendchar2(0X1A);break;
//	  case GSM_STOP:;break;
//	  case GSM_DISCONNECT:Send_UART2((unsigned char*)"+++");break;	//"AT+CIPCLOSE\r\n"
//	  // PROVJERITI !!!case GSM_IDLE:  printf("IDLE %d\r\n",bit_get(FIO3PIN,LONGBIT(GSM_STAT)));   break;
//  }
//  return (routers.gprs.gprs_status);
//}
//
//