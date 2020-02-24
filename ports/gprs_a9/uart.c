/*
 * This file is part of the MicroPython project, http://micropython.org/
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

#include "uart.h"
#include "mphalport.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/ringbuf.h"

#include "api_hal_uart.h"

// ------------------------------
// Hardware UART static variables
// ------------------------------

// The two ports
static const UART_Port_t uart_port[] = {UART1, UART2};

// rx data buffers
uint8_t uart1_ringbuf_array[UART_STATIC_RXBUF_LEN];
uint8_t uart2_ringbuf_array[UART_STATIC_RXBUF_LEN];

uint8_t *uart_ringbuf_array[] = {uart1_ringbuf_array, uart2_ringbuf_array};

// rx ring buffers
ringbuf_t uart_ringbuf[] = {
    {uart1_ringbuf_array, sizeof(uart1_ringbuf_array), 0, 0},
    {uart2_ringbuf_array, sizeof(uart2_ringbuf_array), 0, 0},
};

// UART rx interrupt handler
static void uart_rx_intr_handler(UART_Callback_Param_t param);

// UART configurations
UART_Config_t uart_dev[] = {
    {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .rxCallback = uart_rx_intr_handler,
        .useEvent = false,
    },
    {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .rxCallback = uart_rx_intr_handler,
        .useEvent = false,
    }
};

// -----------------------
// Hardware UART functions
// -----------------------

void soft_reset(void);
void mp_keyboard_interrupt(void);

static bool uart_config(uint8_t uart) {
    // Updates the configuration of UART port
    return UART_Init(uart_port[uart], uart_dev[uart]);
}

uint32_t uart_tx_one_char(uint8_t uart, char c) {
    // Transfers a single char
    return UART_Write(uart_port[uart], (uint8_t*) &c, 1);
}

bool uart_close(uint8_t uart) {
    return UART_Close(uart_port[uart]);
}

static void uart_rx_intr_handler(UART_Callback_Param_t param) {
    // handles rx interrupts
    ringbuf_t *ringbuf = uart_ringbuf + param.port - 1;
    int* to_dupterm = uart_attached_to_dupterm + param.port - 1;
    for (uint32_t i=0; i<param.length; i++) {
        if (param.buf[i] == mp_interrupt_char) {
            if (*to_dupterm)
                mp_keyboard_interrupt();
        }
        else
            ringbuf_put(ringbuf, param.buf[i]);
    }
}

bool uart_rx_wait(uint8_t uart, uint32_t timeout_us) {
    // waits for rx to become populated
    uint32_t start = mp_hal_ticks_us();
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    for (;;) {
        if (ringbuf->iget != ringbuf->iput) {
            return true; // have at least 1 char ready for reading
        }
        if (mp_hal_ticks_us() - start >= timeout_us) {
            return false; // timeout
        }
    }
}

int uart_rx_any(uint8_t uart) {
    // checks if rx is not empty
    ringbuf_t *ringbuf = uart_ringbuf + uart;
    if (ringbuf->iget != ringbuf->iput) {
        return true;
    }
    return false;
}

int uart_tx_any_room(uint8_t uart) {
    // checks if ready for tx
    (void) uart;
    return true;
}

int uart_rx_char(uint8_t uart) {
    // receives a single byte
    return ringbuf_get(uart_ringbuf + uart);
}

void uart_init(UART_Baud_Rate_t uart1_br, UART_Baud_Rate_t uart2_br) {
    // just init with the baud rate
    uart_dev[0].baudRate = uart1_br;
    uart_config(0);
    uart_dev[1].baudRate = uart2_br;
    uart_config(1);
    // install handler for "os" messages
    // os_install_putc1((void *)uart_os_write_char);
}

bool uart_setup(uint8_t uart) {
    return uart_config(uart);
}

int uart_get_rxbuf_len(uint8_t uart) {
    return uart_ringbuf[uart].size;
}

void uart_set_rxbuf(uint8_t uart, uint8_t *buf, int len) {
    uart_ringbuf[uart].buf = buf;
    uart_ringbuf[uart].size = len;
    uart_ringbuf[uart].iget = 0;
    uart_ringbuf[uart].iput = 0;
}

// Task-based UART interface
/*
#include "py/obj.h"
#include "lib/utils/pyexec.h"

#if MICROPY_REPL_EVENT_DRIVEN
void uart_task_handler(os_event_t *evt) {
    if (pyexec_repl_active) {
        // TODO: Just returning here isn't exactly right.
        // What really should be done is something like
        // enquing delayed event to itself, for another
        // chance to feed data to REPL. Otherwise, there
        // can be situation when buffer has bunch of data,
        // and sits unprocessed, because we consumed all
        // processing signals like this.
        return;
    }

    int c, ret = 0;
    while ((c = ringbuf_get(&stdin_ringbuf)) >= 0) {
        if (c == mp_interrupt_char) {
            mp_keyboard_interrupt();
        }
        ret = pyexec_event_repl_process_char(c);
        if (ret & PYEXEC_FORCED_EXIT) {
            break;
        }
    }

    if (ret & PYEXEC_FORCED_EXIT) {
        soft_reset();
    }
}

void uart_task_init() {
    system_os_task(uart_task_handler, UART_TASK_ID, uart_evt_queue, sizeof(uart_evt_queue) / sizeof(*uart_evt_queue));
}
#endif */

