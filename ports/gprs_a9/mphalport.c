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
#include "uart.h"

#include "py/runtime.h"
#include "extmod/misc.h"

int uart_attached_to_dupterm[UART_NPORTS];

int mp_hal_stdin_rx_chr(void) {
    // This should be blocking
    for (;;) {
        int c = mp_uos_dupterm_rx_chr();
        if (c != -1) {
            return c;
        }
        // This has to be sufficiently large to perform background tasks
        OS_Sleep(10);
        mp_handle_pending();
    }
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_uos_dupterm_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_strn(const char *str, uint32_t len) {
    mp_uos_dupterm_tx_strn(str, len);
}

void mp_hal_stdout_tx_strn_cooked(const char *str, uint32_t len) {
    const char *last = str;
    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                mp_uos_dupterm_tx_strn(last, str - last);
            }
            mp_uos_dupterm_tx_strn("\r\n", 2);
            ++str;
            last = str;
        } else {
            ++str;
        }
    }
    if (str > last) {
        mp_uos_dupterm_tx_strn(last, str - last);
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
        MICROPY_EVENT_POLL_HOOK
        OS_SleepUs(1);
    }
}

void mp_hal_delay_us_fast(uint32_t us) {
    OS_SleepUs(us);
}

