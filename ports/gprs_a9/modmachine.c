/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Paul Sokolovsky
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

#include "modmachine.h"

#include "mpconfigport.h"
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "api_event.h"
#include "api_hal_pm.h"
#include "api_hal_watchdog.h"

// ------
// Notify
// ------

Power_On_Cause_t powerOnCause = POWER_ON_CAUSE_MAX;

    void notify_power_on(API_Event_t* event) {
        powerOnCause = event->param1;
}

// -------
// Methods
// -------

STATIC mp_obj_t reset(void) {
    // ========================================
    // Resets the module.
    // ========================================
    PM_Restart();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(reset_obj, reset);

STATIC mp_obj_t idle(void) {
    // ========================================
    // Puts the module into low-power mode.
    // The execution continues.
    // ========================================
    PM_SleepMode(true);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(idle_obj, idle);

STATIC mp_obj_t power_on_cause(void) {
    // ========================================
    // Retrieves the last reason for the power on.
    // Returns:
    //     An integer with the reason encoded.
    // ========================================
    switch (powerOnCause) {
        case POWER_ON_CAUSE_KEY:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_KEY);
            break;
        case POWER_ON_CAUSE_CHARGE:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_CHARGE);
            break;
        case POWER_ON_CAUSE_ALARM:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_ALARM);
            break;
        case POWER_ON_CAUSE_EXCEPTION:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_EXCEPTION);
            break;
        case POWER_ON_CAUSE_RESET:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_RESET);
            break;
        case POWER_ON_CAUSE_MAX:
            return MP_OBJ_NEW_SMALL_INT(POWER_ON_CAUSE_MAX);
            break;
        default:
            return mp_obj_new_int(powerOnCause);
            break;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(power_on_cause_obj, power_on_cause);

STATIC mp_obj_t off(void) {
    // ========================================
    // Turns off the module.
    // ========================================
    PM_ShutDown();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(off_obj, off);

STATIC mp_obj_t get_input_voltage(void) {
    // ========================================
    // Retrieves the input voltage and the
    // estimated battery capacity in percents.
    // Returns:
    //     Two integers: the voltage value in mV
    //     as well as the estimated battery capacity in percents.
    // ========================================
    uint8_t percentage;
    uint16_t voltage = PM_Voltage(&percentage);
    mp_obj_t tuple[2] = {
        mp_obj_new_int(voltage),
        mp_obj_new_int(percentage),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_input_voltage_obj, get_input_voltage);

STATIC mp_obj_t watchdog_on(mp_obj_t timeout) {
    // ========================================
    // Arms the hardware watchdog.
    // Args:
    //     timeout (int): timeout in seconds.
    // ========================================
    WatchDog_Open(WATCHDOG_SECOND_TO_TICK(mp_obj_get_int(timeout)));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(watchdog_on_obj, watchdog_on);

STATIC mp_obj_t watchdog_off(void) {
    // ========================================
    // Disarms the hardware watchdog.
    // ========================================
    WatchDog_Close();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(watchdog_off_obj, watchdog_off);

STATIC mp_obj_t watchdog_reset(void) {
    // ========================================
    // Resets the watchdog timeout.
    // ========================================
    WatchDog_KeepAlive();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(watchdog_reset_obj, watchdog_reset);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_umachine) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_idle), (mp_obj_t)&idle_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_on_cause), (mp_obj_t)&power_on_cause_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_input_voltage), (mp_obj_t)&get_input_voltage_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_on), (mp_obj_t)&watchdog_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_off), (mp_obj_t)&watchdog_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_reset), (mp_obj_t)&watchdog_reset_obj },

    // Reset reasons
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_KEY),       MP_ROM_INT(POWER_ON_CAUSE_KEY) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_CHARGE),    MP_ROM_INT(POWER_ON_CAUSE_CHARGE) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_ALARM),     MP_ROM_INT(POWER_ON_CAUSE_ALARM) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_EXCEPTION), MP_ROM_INT(POWER_ON_CAUSE_EXCEPTION) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_RESET),     MP_ROM_INT(POWER_ON_CAUSE_RESET) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_MAX),       MP_ROM_INT(POWER_ON_CAUSE_MAX) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};


