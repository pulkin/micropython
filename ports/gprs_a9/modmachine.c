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

#include "modmachine.h"
#include "extmod/machine_spi.h"

#include "mpconfigport.h"
#include "stdint.h"
#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"

#include "api_event.h"
#include "api_hal_pm.h"
#include "api_hal_watchdog.h"
#include "api_hal_adc.h"

STATIC mp_obj_t modmachine_watchdog_off(void);
STATIC mp_obj_t power_key_callback = mp_const_none;

void modmachine_init0(void) {
    PM_SetSysMinFreq(PM_SYS_FREQ_312M);
    modmachine_watchdog_off();
    modmachine_pin_init0();
    modmachine_uart_init0();
    power_key_callback = mp_const_none;
}

// ------
// Notify
// ------

Power_On_Cause_t powerOnCause = POWER_ON_CAUSE_MAX;

void modmachine_notify_power_on(API_Event_t* event) {
    powerOnCause = event->param1;
}

void modmachine_notify_power_key_down(API_Event_t* event) {
    if (power_key_callback && power_key_callback != mp_const_none) {
        mp_sched_schedule(power_key_callback, mp_obj_new_bool(1));
    }
}
 
void modmachine_notify_power_key_up(API_Event_t* event) {
    if (power_key_callback && power_key_callback != mp_const_none) {
        mp_sched_schedule(power_key_callback, mp_obj_new_bool(0));
    }
}

// -------
// Methods
// -------

STATIC mp_obj_t modmachine_reset(void) {
    // ========================================
    // Resets the module.
    // ========================================
    PM_Restart();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_reset_obj, modmachine_reset);

STATIC mp_obj_t modmachine_set_idle(mp_obj_t flag) {
    // ========================================
    // Puts the module into low-power mode by
    // tuning down frequencies and powering
    // down certain peripherials.
    // The execution continues.
    // Args:
    //     flag (bool): if True, triggers low-
    //     power mode. If False, switches low-
    //     power mode off.
    // ========================================
    PM_SleepMode(mp_obj_get_int(flag));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modmachine_set_idle_obj, modmachine_set_idle);

STATIC mp_obj_t modmachine_set_min_freq(mp_obj_t freq) {
    // ========================================
    // Sets the minimal CPU frequency.
    // Args:
    //     freq (int): a constant specifying
    //     the frequency.
    // ========================================
    int i = mp_obj_get_int(freq);
    if (i != PM_SYS_FREQ_32K && i != PM_SYS_FREQ_13M && i != PM_SYS_FREQ_26M && i != PM_SYS_FREQ_39M &&
            i != PM_SYS_FREQ_52M && i != PM_SYS_FREQ_78M && i != PM_SYS_FREQ_89M && i != PM_SYS_FREQ_104M &&
            i != PM_SYS_FREQ_113M && i != PM_SYS_FREQ_125M && i != PM_SYS_FREQ_139M && i != PM_SYS_FREQ_156M &&
            i != PM_SYS_FREQ_178M && i != PM_SYS_FREQ_208M && i != PM_SYS_FREQ_250M && i != PM_SYS_FREQ_312M)
        mp_raise_ValueError("Unknown frequency");
    PM_SetSysMinFreq(i);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modmachine_set_min_freq_obj, modmachine_set_min_freq);

STATIC mp_obj_t modmachine_power_on_cause(void) {
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

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_power_on_cause_obj, modmachine_power_on_cause);

STATIC mp_obj_t modmachine_off(void) {
    // ========================================
    // Turns off the module.
    // ========================================
    PM_ShutDown();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_off_obj, modmachine_off);

STATIC mp_obj_t modmachine_get_input_voltage(void) {
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

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_get_input_voltage_obj, modmachine_get_input_voltage);

STATIC mp_obj_t modmachine_watchdog_on(mp_obj_t timeout) {
    // ========================================
    // Arms the hardware watchdog.
    // Args:
    //     timeout (int): timeout in seconds.
    // ========================================
    WatchDog_Open(WATCHDOG_SECOND_TO_TICK(mp_obj_get_int(timeout)));
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modmachine_watchdog_on_obj, modmachine_watchdog_on);

STATIC mp_obj_t modmachine_watchdog_off(void) {
    // ========================================
    // Disarms the hardware watchdog.
    // ========================================
    WatchDog_Close();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_watchdog_off_obj, modmachine_watchdog_off);

STATIC mp_obj_t modmachine_watchdog_reset(void) {
    // ========================================
    // Resets the watchdog timeout.
    // ========================================
    WatchDog_KeepAlive();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modmachine_watchdog_reset_obj, modmachine_watchdog_reset);

STATIC mp_obj_t modmachine_on_power_key(mp_obj_t callable) {
    // ========================================
    // Sets a callback on power key press.
    // Args:
    //     callback (Callable): a callback to
    //     execute on power key press.
    // ========================================
    power_key_callback = callable;
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(modmachine_on_power_key_obj, modmachine_on_power_key);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_umachine) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },
    { MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&machine_adc_type) },
    { MP_ROM_QSTR(MP_QSTR_UART), MP_ROM_PTR(&pyb_uart_type) },
    { MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&pyb_rtc_type) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_reset), (mp_obj_t)&modmachine_reset_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_idle), (mp_obj_t)&modmachine_set_idle_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_set_min_freq), (mp_obj_t)&modmachine_set_min_freq_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_power_on_cause), (mp_obj_t)&modmachine_power_on_cause_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_power_key), (mp_obj_t)&modmachine_on_power_key_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&modmachine_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_input_voltage), (mp_obj_t)&modmachine_get_input_voltage_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_on), (mp_obj_t)&modmachine_watchdog_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_off), (mp_obj_t)&modmachine_watchdog_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_watchdog_reset), (mp_obj_t)&modmachine_watchdog_reset_obj },
    #if MICROPY_PY_MACHINE_SPI
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&mp_machine_soft_spi_type) },
    #endif

    // Reset reasons
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_KEY),       MP_ROM_INT(POWER_ON_CAUSE_KEY) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_CHARGE),    MP_ROM_INT(POWER_ON_CAUSE_CHARGE) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_ALARM),     MP_ROM_INT(POWER_ON_CAUSE_ALARM) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_EXCEPTION), MP_ROM_INT(POWER_ON_CAUSE_EXCEPTION) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_RESET),     MP_ROM_INT(POWER_ON_CAUSE_RESET) },
    { MP_ROM_QSTR(MP_QSTR_POWER_ON_CAUSE_MAX),       MP_ROM_INT(POWER_ON_CAUSE_MAX) },

    // Frequencies
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_32K),          MP_ROM_INT(PM_SYS_FREQ_32K) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_13M),          MP_ROM_INT(PM_SYS_FREQ_13M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_26M),          MP_ROM_INT(PM_SYS_FREQ_26M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_39M),          MP_ROM_INT(PM_SYS_FREQ_39M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_52M),          MP_ROM_INT(PM_SYS_FREQ_52M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_78M),          MP_ROM_INT(PM_SYS_FREQ_78M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_89M),          MP_ROM_INT(PM_SYS_FREQ_89M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_104M),         MP_ROM_INT(PM_SYS_FREQ_104M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_113M),         MP_ROM_INT(PM_SYS_FREQ_113M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_125M),         MP_ROM_INT(PM_SYS_FREQ_125M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_139M),         MP_ROM_INT(PM_SYS_FREQ_139M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_156M),         MP_ROM_INT(PM_SYS_FREQ_156M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_178M),         MP_ROM_INT(PM_SYS_FREQ_178M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_208M),         MP_ROM_INT(PM_SYS_FREQ_208M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_250M),         MP_ROM_INT(PM_SYS_FREQ_250M) },
    { MP_ROM_QSTR(MP_QSTR_PM_SYS_FREQ_312M),         MP_ROM_INT(PM_SYS_FREQ_312M) },

    // ADC poll period
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_122US),  MP_ROM_INT(ADC_SAMPLE_PERIOD_122US) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_1MS),    MP_ROM_INT(ADC_SAMPLE_PERIOD_1MS) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_10MS),   MP_ROM_INT(ADC_SAMPLE_PERIOD_10MS) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_100MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_100MS) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_250MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_250MS) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_500MS),  MP_ROM_INT(ADC_SAMPLE_PERIOD_500MS) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_1S),     MP_ROM_INT(ADC_SAMPLE_PERIOD_1S) },
    { MP_ROM_QSTR(MP_QSTR_ADC_SAMPLE_PERIOD_2S),     MP_ROM_INT(ADC_SAMPLE_PERIOD_2S) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};


