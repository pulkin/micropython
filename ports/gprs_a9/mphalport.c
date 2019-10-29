/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 pulkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

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

uint32_t mp_hal_ticks_ms(void) {
    return (uint32_t)(clock() / CLOCKS_PER_MSEC);
}

uint32_t mp_hal_ticks_us(void) {
    return (uint32_t)(clock() / CLOCKS_PER_MSEC * 1000);
}

void mp_hal_delay_ms(uint32_t ms) {
    uint32_t start = clock();
    while (clock() - start < ms * CLOCKS_PER_MSEC) {
        OS_Sleep(1);
        mp_handle_pending();
    }
}

void mp_hal_delay_us(uint32_t us) {
    uint32_t start = clock();
    while ((clock() - start) * 1000 < us * CLOCKS_PER_MSEC) {
        OS_SleepUs(1);
        mp_handle_pending();
    }
}

void mp_hal_delay_us_fast(uint32_t us) {
    OS_SleepUs(us);
}

