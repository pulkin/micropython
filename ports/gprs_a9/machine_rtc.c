/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
 * Copyright (c) 2020 pulkin
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
#include "time.h"
#include "errno.h"

#include "py/runtime.h"
#include "modmachine.h"

typedef struct _pyb_rtc_obj_t {
    mp_obj_base_t base;
} pyb_rtc_obj_t;

// singleton RTC object
STATIC const pyb_rtc_obj_t pyb_rtc_obj = {{&pyb_rtc_type}};

void mp_hal_rtc_init(void) {
}

STATIC mp_obj_t pyb_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // check arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // return constant object
    return (mp_obj_t)&pyb_rtc_obj;
}

STATIC mp_obj_t pyb_rtc_datetime(size_t n_args, const mp_obj_t *args) {
    if (n_args == 1) {
        // Get time
        RTC_Time_t tm;
        if (!TIME_GetRtcTime(&tm))
            mp_raise_OSError(EIO);
        mp_obj_t tuple[8] = {
            mp_obj_new_int(tm.year),
            mp_obj_new_int(tm.month),
            mp_obj_new_int(tm.day),
            mp_obj_new_int(0),
            mp_obj_new_int(tm.hour),
            mp_obj_new_int(tm.minute),
            mp_obj_new_int(tm.second),
            mp_obj_new_int(0)
        };

        return mp_obj_new_tuple(8, tuple);
    } else {
        // Set time
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);

        RTC_Time_t tm;
        tm.year = mp_obj_get_int(items[0]);
        tm.month = mp_obj_get_int(items[1]);
        tm.day = mp_obj_get_int(items[2]);
        tm.hour = mp_obj_get_int(items[4]);
        tm.minute = mp_obj_get_int(items[5]);
        tm.second = mp_obj_get_int(items[6]);

        if (!TIME_SetRtcTime(&tm))
            mp_raise_OSError(EIO);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_datetime_obj, 1, 2, pyb_rtc_datetime);

STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&pyb_rtc_datetime_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);

const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_rtc_locals_dict,
};

