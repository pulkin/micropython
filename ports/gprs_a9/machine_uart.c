/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "mphalport.h"
#include "modmachine.h"
#include "moduos.h"

STATIC const char *_parity_name[] = {"None", "1", "0"};

void modmachine_uart_init0(void) {
    UART_Close(UART2);
    UART_Close(UART1);

    uart_attached_to_dupterm[0] = 0;
    uart_attached_to_dupterm[1] = 0;

    // Attach UART to dupterm(1)
    mp_obj_t args[2];
    args[0] = MP_OBJ_NEW_SMALL_INT(0);
    args[1] = MP_OBJ_NEW_SMALL_INT(115200);
    args[0] = pyb_uart_type.make_new(&pyb_uart_type, 2, 0, args);
    args[1] = MP_OBJ_NEW_SMALL_INT(1);
    // extern mp_obj_t mp_os_dupterm(size_t n_args, const mp_obj_t *args);
    os_dupterm(2, args);
}

/******************************************************************************/
// MicroPython bindings for UART

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, rxbuf=%u, timeout=%u, timeout_char=%u)",
        self->uart_id, self->baudrate, self->bits, _parity_name[self->parity],
        self->stop, uart_get_rxbuf_len(self->uart_id) - 1, self->timeout, self->timeout_char);
}

STATIC void pyb_uart_init_helper(pyb_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_tx, ARG_rx, ARG_rxbuf, ARG_timeout, ARG_timeout_char };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_baudrate, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_bits, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_parity, MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_stop, MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_tx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rx, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_rxbuf, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout_char, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
    };
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // set tx/rx pins
    if (args[ARG_tx].u_obj != MP_OBJ_NULL && args[ARG_rx].u_obj != MP_OBJ_NULL) {
        mp_hal_pin_obj_t tx = mp_hal_get_pin_obj(args[ARG_tx].u_obj);
        mp_hal_pin_obj_t rx = mp_hal_get_pin_obj(args[ARG_rx].u_obj);
        if (tx == 1 && rx == 0) {
            self->uart_id = 0;
        } else if (tx == 4 && rx == 5) {
            self->uart_id = 1;
        } else {
            mp_raise_ValueError("invalid tx/rx");
        }
    }

    UART_Config_t *config = uart_dev + self->uart_id;

    // set baudrate
    if (args[ARG_baudrate].u_int > 0) {
        switch (args[ARG_baudrate].u_int) {
            case UART_BAUD_RATE_1200:
            case UART_BAUD_RATE_2400:
            case UART_BAUD_RATE_4800:
            case UART_BAUD_RATE_9600:
            case UART_BAUD_RATE_14400:
            case UART_BAUD_RATE_19200:
            case UART_BAUD_RATE_28800:
            case UART_BAUD_RATE_33600:
            case UART_BAUD_RATE_38400:
            case UART_BAUD_RATE_57600:
            case UART_BAUD_RATE_115200:
            case UART_BAUD_RATE_230400:
            case UART_BAUD_RATE_460800:
            case UART_BAUD_RATE_921600:
            case UART_BAUD_RATE_1300000:
            case UART_BAUD_RATE_1625000:
            case UART_BAUD_RATE_2166700:
            case UART_BAUD_RATE_3250000:
                config->baudRate = (UART_Baud_Rate_t) args[ARG_baudrate].u_int;
            case 0:
                break;
            default:
                mp_raise_ValueError("unsupported baud rate");
                break;
        }
    }
    self->baudrate = config->baudRate;

    // set data bits
    switch (args[ARG_bits].u_int) {
        case 7:
        case 8:
            config->dataBits = (UART_Data_Bits_t) args[ARG_bits].u_int;
        case 0:
            break;
        default:
            mp_raise_ValueError("invalid data bits");
            break;
    }
    self->bits = config->dataBits;

    // set parity
    if (args[ARG_parity].u_obj != MP_OBJ_NULL) {
        if (args[ARG_parity].u_obj == mp_const_none) {
            config->parity = UART_PARITY_NONE;
        } else {
            mp_int_t parity = mp_obj_get_int(args[ARG_parity].u_obj);
            if (parity & 1) {
                config->parity = UART_PARITY_ODD;
            } else {
                config->parity = UART_PARITY_EVEN;
            }
        }
    }
    self->parity = config->parity;

    // set stop bits
    switch (args[ARG_stop].u_int) {
        case 1:
            config->stopBits = UART_STOP_BITS_1;
            break;
        case 2:
            config->stopBits = UART_STOP_BITS_2;
            break;
        case 0:
            break;
        default:
            mp_raise_ValueError("invalid stop bits");
            break;
    }
    self->stop = config->stopBits + 1;

    // set rx ring buffer
    if (args[ARG_rxbuf].u_int > 0) {
        uint16_t len = args[ARG_rxbuf].u_int + 1; // account for usable items in ringbuf
        uint8_t *buf;
        if (len <= UART_STATIC_RXBUF_LEN) {
            buf = uart_ringbuf_array[self->uart_id];
            MP_STATE_PORT(uart_rxbuf[self->uart_id]) = NULL; // clear any old pointer
        } else {
            buf = m_new(uint8_t, len);
            MP_STATE_PORT(uart_rxbuf[self->uart_id]) = buf; // retain root pointer
        }
        uart_set_rxbuf(self->uart_id, buf, len);
    }

    // set timeout
    self->timeout = args[ARG_timeout].u_int;

    // set timeout_char
    // make sure it is at least as long as a whole character (13 bits to be safe)
    self->timeout_char = args[ARG_timeout_char].u_int;
    uint32_t min_timeout_char = 13000 / self->baudrate + 1;
    if (self->timeout_char < min_timeout_char) {
        self->timeout_char = min_timeout_char;
    }

    // setup
    if (!uart_setup(self->uart_id))
        mp_raise_ValueError("failed to setup uart");
}

STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get uart id
    mp_int_t uart_id = mp_obj_get_int(args[0]);
    if (uart_id < 0 || uart_id >= UART_NPORTS) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "UART(%d) does not exist", uart_id));
    }

    // create instance
    pyb_uart_obj_t *self = m_new_obj(pyb_uart_obj_t);
    self->base.type = &pyb_uart_type;
    self->uart_id = uart_id;
    self->baudrate = 115200;
    self->bits = 8;
    self->parity = 0;
    self->stop = 1;
    self->timeout = 0;
    self->timeout_char = 0;

    // init the peripheral
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    pyb_uart_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t pyb_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    pyb_uart_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(uart_rx_any(self->uart_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

STATIC const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&pyb_uart_init_obj) },

    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&pyb_uart_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // wait for first char to become available
    if (!uart_rx_wait(self->uart_id, self->timeout * 1000)) {
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    uint8_t *buf = buf_in;
    for (;;) {
        *buf++ = uart_rx_char(self->uart_id);
        if (--size == 0 || !uart_rx_wait(self->uart_id, self->timeout_char * 1000)) {
            // return number of bytes read
            return buf - (uint8_t*)buf_in;
        }
    }
}

STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const byte *buf = buf_in;

    /* TODO implement non-blocking
    // wait to be able to write the first character
    if (!uart_tx_wait(self, timeout)) {
        *errcode = EAGAIN;
        return MP_STREAM_ERROR;
    }
    */

    // write the data
    for (size_t i = 0; i < size; ++i) {
        uart_tx_one_char(self->uart_id, *buf++);
    }

    // return number of bytes written
    return size;
}

STATIC mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;
    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self->uart_id)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && uart_tx_any_room(self->uart_id)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else if (request == MP_STREAM_CLOSE) {
        uart_close(self->uart_id);
        ret = 0;
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};

const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = pyb_uart_print,
    .make_new = pyb_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_dict_t*)&pyb_uart_locals_dict,
};
