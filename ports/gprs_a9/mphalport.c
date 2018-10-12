
#include "mpconfigport.h"

#include "time.h"
#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"
#include "buffer.h"
#include "time.h"





bool UartInit()
{
    UART_Config_t config = {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity   = UART_PARITY_NONE,
        .rxCallback = NULL,
        .useEvent = true,
    };
    UART_Init(UART1,config);
    return true;
}



// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    UART_Write(UART1,(uint8_t*)str,len);
}

// Send zero-terminated string
void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

// Efficiently convert "\n" to "\r\n"
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    const char *last = str;
    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                mp_hal_stdout_tx_strn(last, str - last);
            }
            mp_hal_stdout_tx_strn("\r\n", 2);
            ++str;
            last = str;
        } else {
            ++str;
        }
    }
    if (str > last) {
        mp_hal_stdout_tx_strn(last, str - last);
    }
}

/////////////////////////////////////////////////////////////////////////////
/////hal time////////////////////////////////////////////////////////////////


uint32_t mp_hal_ticks_ms(void) {
    return (uint32_t)(clock()/CLOCKS_PER_MSEC);
}

uint32_t mp_hal_ticks_us(void) {
    return (uint32_t)(clock()/CLOCKS_PER_MSEC*1000);
}

void mp_hal_delay_ms(uint32_t ms) {
    OS_Sleep(ms);
}

void mp_hal_delay_us(uint32_t us) {
    OS_SleepUs(us);
}

// this function could do with improvements (eg use ets_delay_us)
void mp_hal_delay_us_fast(uint32_t us) {
    OS_SleepUs(us);
}


/////////////////////////////////////////////////////////////////////////////


