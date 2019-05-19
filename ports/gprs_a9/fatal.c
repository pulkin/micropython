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

#include <stdio.h>
#include <string.h>

#include "api_os.h"
#include "api_fs.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"

#include "mpconfigport.h"
#include "fatal.h"

static const char msg1[] =
    "\r\n==============================="
    "\r\nMicropython experienced a fatal"
    "\r\n   error and will be halted."
    "\r\n"
    "\r\n  reason: ";
static const char msg2[] =
    "\r\n  ptr1: ";
static const char msg3[] =
    "\r\n=============================="
    "\r\n";

static const char msg_nlr_jump_fail[] = "nlr jump fail";
static const char msg_heap_init[] = "heap init fail";
static const char msg_unknown[] = "unknown";

char var[16];

void NORETURN mp_fatal_error(uint8_t reason, void* ptr1) {
    // ========================================
    // A fatal error. Avoid using stack.
    // ========================================
    UART_Write(UART1, (uint8_t*)msg1, sizeof(msg1));
    switch (reason) {
        case MP_FATAL_REASON_NLR_JUMP_FAIL:
            UART_Write(UART1, (uint8_t*)msg_nlr_jump_fail, sizeof(msg_nlr_jump_fail));
            break;
        case MP_FATAL_REASON_HEAP_INIT:
            UART_Write(UART1, (uint8_t*)msg_heap_init, sizeof(msg_heap_init));
            break;
        default:
            UART_Write(UART1, (uint8_t*)msg_unknown, sizeof(msg_unknown));
    }
    UART_Write(UART1, (uint8_t*)msg2, sizeof(msg2));
    snprintf(var, sizeof(var), "%p", ptr1);
    UART_Write(UART1, (uint8_t*)var, strlen(var)+1);
    UART_Write(UART1, (uint8_t*)msg3, sizeof(msg3));

    int fd = API_FS_Open(".reboot_on_fatal", FS_O_RDONLY, 0);
    if (fd < 0) {
        switch (reason) {
            case MP_FATAL_REASON_NLR_JUMP_FAIL:
                Assert(false, msg_nlr_jump_fail);
                break;
            case MP_FATAL_REASON_HEAP_INIT:
                Assert(false, msg_heap_init);
                break;
            default:
                Assert(false, msg_unknown);
        }
    } else {
        API_FS_Close(fd);
        OS_Sleep(5000);
        PM_Restart();
    }
    while (1);
}

