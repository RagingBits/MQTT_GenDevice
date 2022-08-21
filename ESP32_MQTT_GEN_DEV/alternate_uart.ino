#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Ticker.h>



/* Alternate Uart Definitions -------------------------------------------------- */
#define ALTERNATE_UART_CLIENT_THREAD_RATE           500U/*milliseconds*/
#define ALTERNATE_UART_UART_INPUT_MQTT_LEN          128U

typedef struct
{
    uint32_t initialised:1U;
    uint32_t work_run:1U;
    uint32_t unused:30U;
}alternate_uart_flags_t;

/* Alternate Uart Constants ---------------------------------------------------- */


/* Alternate Uart Variables ---------------------------------------------------- */
alternate_uart_flags_t alternate_uart_flags = {0U};

/* Alternate Uart Clients ------------------------------------------------------ */
Ticker alternate_uart_client_thread;

/* Alternate Uart External Functions ------------------------------------------- */
void AlternateUartStart(void)
{
    if(0U == alternate_uart_flags.initialised)
    {
        alternate_uart_client_thread.attach_ms(ALTERNATE_UART_CLIENT_THREAD_RATE, AlternateUartRunWork);
        alternate_uart_flags.initialised = !0U;
    }  
}

void AlternateUartRunWork(void)
{
    alternate_uart_flags.work_run = !0U;
}

void AlternateUartWork(void)
{
    if(0U != alternate_uart_flags.work_run)
    {
        if(IOPinsAltUartIsEn())
        {
            uint8_t data_in[ALTERNATE_UART_UART_INPUT_MQTT_LEN]= {0U};
            size_t read_len = Serial1.read(data_in, ALTERNATE_UART_UART_INPUT_MQTT_LEN);
            if(0U != read_len)
            {
                  MQTTHandleUartInputMessage(data_in, read_len);
            }
        }
    }
}

/* Alternate Uart Internal Functions ------------------------------------------- */
