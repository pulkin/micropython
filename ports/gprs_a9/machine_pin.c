/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Damien P. George
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

#include "py/runtime.h"
#include "py/mphal.h"
#include "mphalport.h"
#include "modmachine.h"
#include "extmod/virtpin.h"

#include "api_hal_pm.h"
#include "api_hal_gpio.h"

#define GPIO_LEVEL_UNSET 255
// #define GPIO_Set(id, val) do {g_InterfaceVtbl->GPIO_Set((id), (val)); if ((id) == 17) {if ((val) == 0) mp_printf(&mp_plat_print, "comm"); else mp_printf(&mp_plat_print, "data");} else if ((id) == 15) {if ((val) == 0) mp_printf(&mp_plat_print, "("); else mp_printf(&mp_plat_print, ")\n");} else if ((id) == 16) {if ((val) == 0) mp_printf(&mp_plat_print, "-"); else mp_printf(&mp_plat_print, "+");} else if ((id) == 18) {if ((val) == 0) mp_printf(&mp_plat_print, "0"); else mp_printf(&mp_plat_print, "1");} else mp_printf(&mp_plat_print, "%d:%d ", (id), (val));} while(0)
void modmachine_pin_init0(void) {
    PM_PowerEnable(POWER_TYPE_MMC, false);
    PM_PowerEnable(POWER_TYPE_LCD, false);
    PM_PowerEnable(POWER_TYPE_CAM, false);
    memset(&MP_STATE_PORT(machine_pin_irq_handler[0]), 0, sizeof(MP_STATE_PORT(machine_pin_irq_handler)));
    memset(&MP_STATE_PORT(machine_pin_irq_handler_arg[0]), 0, sizeof(MP_STATE_PORT(machine_pin_irq_handler_arg)));
}

// -------
// Classes
// -------

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    uint32_t phys_port;
    GPIO_PIN id;
} machine_pin_obj_t;

typedef struct _machine_pin_irq_obj_t {
    mp_obj_base_t base;
    uint32_t phys_port;
    GPIO_PIN id;
} machine_pin_irq_obj_t;

STATIC const machine_pin_obj_t machine_pin_obj[] = {
    {{&machine_pin_type},  0, GPIO_PIN0},
    {{&machine_pin_type},  1, GPIO_PIN1},
    {{&machine_pin_type},  2, GPIO_PIN2},
    {{&machine_pin_type},  3, GPIO_PIN3},
    {{&machine_pin_type},  4, GPIO_PIN4},
    {{&machine_pin_type},  5, GPIO_PIN5},
    {{&machine_pin_type},  6, GPIO_PIN6},
    {{&machine_pin_type},  7, GPIO_PIN7},
    {{&machine_pin_type},  8, GPIO_PIN8},
    {{&machine_pin_type},  9, GPIO_PIN9},
    {{&machine_pin_type}, 10, GPIO_PIN10},
    {{&machine_pin_type}, 11, GPIO_PIN11},
    {{&machine_pin_type}, 12, GPIO_PIN12},
    {{&machine_pin_type}, 13, GPIO_PIN13},
    {{&machine_pin_type}, 14, GPIO_PIN14},
    {{&machine_pin_type}, 15, GPIO_PIN15},
    {{&machine_pin_type}, 16, GPIO_PIN16},
    {{&machine_pin_type}, 17, GPIO_PIN17},
    {{&machine_pin_type}, 18, GPIO_PIN18},
    {{&machine_pin_type}, 19, GPIO_PIN19},
    {{&machine_pin_type}, 20, GPIO_PIN20},
    {{&machine_pin_type}, 21, GPIO_PIN21},
    {{&machine_pin_type}, 22, GPIO_PIN22},
    {{&machine_pin_type}, 23, GPIO_PIN23},
    {{&machine_pin_type}, 24, GPIO_PIN24},
    {{&machine_pin_type}, 25, GPIO_PIN25},
    {{&machine_pin_type}, 26, GPIO_PIN26},
    {{&machine_pin_type}, 27, GPIO_PIN27},
    {{&machine_pin_type}, 28, GPIO_PIN28},
    {{&machine_pin_type}, 29, GPIO_PIN29},
    {{&machine_pin_type}, 30, GPIO_PIN30},
    {{&machine_pin_type}, 31, GPIO_PIN31},
    {{&machine_pin_type}, 32, GPIO_PIN32},
    {{&machine_pin_type}, 33, GPIO_PIN33},
    {{&machine_pin_type}, 34, GPIO_PIN34},
};

// forward declaration
STATIC machine_pin_irq_obj_t machine_pin_irq_object[];

void pin_init(GPIO_PIN id, GPIO_MODE mode, GPIO_LEVEL default_level) {
    GPIO_config_t config = {
        .pin=id,
        .mode=mode,
        .defaultLevel=default_level,
    };

    // configure the pin for gpio
    if ((id >= 8) && (id <= 13))
        PM_PowerEnable(POWER_TYPE_MMC, true);
    else if ((id >= 14) && (id <= 18))
        PM_PowerEnable(POWER_TYPE_LCD, true);
    else if ((id >= 19) && (id <= 24))
        PM_PowerEnable(POWER_TYPE_CAM, true);

    GPIO_Init(config);
}

STATIC void machine_pin_isr_handler(void *arg) {
    machine_pin_obj_t *self = arg;
    mp_obj_t handler = MP_STATE_PORT(machine_pin_irq_handler)[self->phys_port];
    mp_obj_t handler_arg = MP_STATE_PORT(machine_pin_irq_handler_arg)[self->phys_port];
    mp_sched_schedule(handler, handler_arg);
}

void machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // ========================================
    // Parses arguments and initializes pins.
    // ========================================
    enum { ARG_mode, ARG_pull, ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_INT, {.u_int = GPIO_MODE_INPUT}},
        { MP_QSTR_pull, MP_ARG_INT, {.u_int = GPIO_LEVEL_UNSET}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = GPIO_LEVEL_UNSET}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    GPIO_LEVEL default_level = GPIO_LEVEL_LOW;

    if (args[ARG_value].u_int != GPIO_LEVEL_UNSET)
        default_level = args[ARG_value].u_int;

    if (args[ARG_pull].u_int != GPIO_LEVEL_UNSET)
        default_level = args[ARG_pull].u_int;

    pin_init(self->id, args[ARG_mode].u_int & GPIO_MODE_INPUT, default_level);
}

mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // ========================================
    // GPIO pin class.
    // Args:
    //     pin (int): integer pointing to a particular pin;
    //     mode (int): input (Pin.IN) or output (Pin.OUT);
    //     value (int): sets the value for output pins;
    //     pull (int): sets internal pull-up resistor for input pins;
    // ========================================
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted pin object
    int wanted_pin = mp_obj_get_int(args[0]);
    const machine_pin_obj_t *self = NULL;
    if (0 <= wanted_pin && wanted_pin < MP_ARRAY_SIZE(machine_pin_obj)) {
        self = (machine_pin_obj_t*)&machine_pin_obj[wanted_pin];
    }
    if (self == NULL || self->base.type == NULL) {
        mp_raise_ValueError("Invalid pin");
    }

    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
    machine_pin_obj_init_helper(self, n_args - 1, args + 1, &kw_args);

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    // ========================================
    // GPIO level setting.
    // Args:
    //     value (bool): GPIO level;
    // Returns:
    //    GPIO level.
    // ========================================
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    GPIO_LEVEL level;

    if (n_args == 0) {
        // get pin
        GPIO_Get(self->id, &level);
        return MP_OBJ_NEW_SMALL_INT(level);
    } else {
        // set pin
        GPIO_Set(self->id, mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    // ========================================
    // Initializes the pin.
    // Args:
    //     mode (int): input (Pin.IN) or output (Pin.OUT);
    //     value (int): sets the value for output pins;
    //     pull (int): sets internal pull-up resistor for input pins;
    // ========================================
    machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Alias of machine_pin_call.
    // ========================================
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    // ========================================
    // Pin.__str__
    // ======================s==================
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "Pin(%u)", self->phys_port);
}

STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GPIO_Set(self->id, 0);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GPIO_Set(self->id, 1);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

void OnInterrupt(GPIO_INT_callback_param_t* param)
{
    int pin_id = param->pin;
    if(pin_id >= 2 && pin_id <= 7){
        mp_obj_t handler = MP_STATE_PORT(machine_pin_irq_handler)[pin_id];
        if (handler != mp_const_none) {
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {  // nlr_push returns 0 if the code runs with no exception and returns non-zero if an exception was raised
                mp_obj_t handler_arg = MP_STATE_PORT(machine_pin_irq_handler_arg)[pin_id];
                if(handler_arg == mp_const_none){
                    mp_sched_schedule(handler, MP_OBJ_NULL);
                }
                else{
                    mp_sched_schedule(handler, handler_arg);
                }
                nlr_pop();
            } else {
                // Uncaught exception; disable the callback so it doesn't run again.
                MP_STATE_PORT(machine_pin_irq_handler)[pin_id] = mp_const_none;
                MP_STATE_PORT(machine_pin_irq_handler_arg)[pin_id] = mp_const_none;
                mp_printf(&mp_plat_print,"There was an exception during the callback");
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            }
        }
    }
}


STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_handler, ARG_trigger, ARG_debounce, ARG_handler_arg };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_handler, MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_trigger, MP_ARG_INT, {.u_int = GPIO_INT_TYPE_FALLING_EDGE} },
        { MP_QSTR_debounce, MP_ARG_INT, {.u_int = 50} },
        { MP_QSTR_handler_arg, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    if (n_args > 1 || kw_args->used != 0) {
        // configure irq
        mp_obj_t handler = args[ARG_handler].u_obj;
        mp_obj_t handler_arg = args[ARG_handler_arg].u_obj;
        uint32_t trigger = args[ARG_trigger].u_int;
        
        uint32_t debounce = args[ARG_debounce].u_int;

        if (handler == mp_const_none) {
            handler = MP_OBJ_NULL;
            trigger = 0;
        }

        GPIO_config_t config;
        GPIO_GetConfig(self->id, &config);
        GPIO_LEVEL default_level = (GPIO_LEVEL)config.defaultLevel;

        MP_STATE_PORT(machine_pin_irq_handler)[self->phys_port] = handler;
        MP_STATE_PORT(machine_pin_irq_handler_arg)[self->phys_port] = handler_arg;

        GPIO_config_t gpioINT = {
            .pin                = self->id,
            .mode               = GPIO_MODE_INPUT_INT,
            .defaultLevel       = default_level,
            .intConfig.debounce = debounce,
            .intConfig.type     = (GPIO_INT_TYPE)trigger,
            .intConfig.callback = OnInterrupt,
        };

        GPIO_Init(gpioINT);
    }

    return MP_OBJ_FROM_PTR(&machine_pin_irq_object[self->phys_port]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);

STATIC mp_uint_t machine_pin_mode(mp_obj_t self_in) {
    machine_pin_irq_obj_t *self = MP_OBJ_FROM_PTR(self_in);
    GPIO_config_t config;
    GPIO_GetConfig(self->id, &config);
    GPIO_MODE mode = (GPIO_MODE)config.mode & GPIO_MODE_INPUT;
    return (mp_int_t) mode;
}

STATIC mp_uint_t machine_pin_pull(mp_obj_t self_in) {
    machine_pin_irq_obj_t *self = MP_OBJ_FROM_PTR(self_in);
    GPIO_config_t config;
    GPIO_GetConfig(self->id, &config);
    GPIO_LEVEL level = (GPIO_LEVEL)config.defaultLevel;
    return (mp_uint_t) level;
}

STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init),  MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value), MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_on),    MP_ROM_PTR(&machine_pin_on_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),   MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),   MP_ROM_PTR(&machine_pin_irq_obj) },

    // attributes
    { MP_ROM_QSTR(MP_QSTR_mode), MP_ROM_PTR(&machine_pin_mode)},
    { MP_ROM_QSTR(MP_QSTR_pull), MP_ROM_PTR(&machine_pin_pull)},

    // class constants
    { MP_ROM_QSTR(MP_QSTR_OUT),       MP_ROM_INT(GPIO_MODE_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_IN),        MP_ROM_INT(GPIO_MODE_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_INT),       MP_ROM_INT(GPIO_MODE_INPUT_INT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(GPIO_LEVEL_LOW) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),   MP_ROM_INT(GPIO_LEVEL_HIGH) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_HIGH),           MP_ROM_INT(GPIO_INT_TYPE_HIGH_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_LOW),            MP_ROM_INT(GPIO_INT_TYPE_LOW_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING),         MP_ROM_INT(GPIO_INT_TYPE_RISING_EDGE) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_FALLING),        MP_ROM_INT(GPIO_INT_TYPE_FALLING_EDGE) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_RISING_FALLING), MP_ROM_INT(GPIO_INT_TYPE_RISING_FALLING_EDGE) },
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);


STATIC mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    GPIO_LEVEL level;

    switch (request) {
        case MP_PIN_READ: {
            GPIO_Get(self->id, &level);
            return level;
        }
        case MP_PIN_WRITE: {
            GPIO_Set(self->id, arg);
            return 0;
        }
    }
    return -1;
}

STATIC const mp_pin_p_t pin_pin_p = {
  .ioctl = pin_ioctl,
};

const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};

/******************************************************************************/
// Pin IRQ object

STATIC const mp_obj_type_t machine_pin_irq_type;

STATIC machine_pin_irq_obj_t machine_pin_irq_object[] = {
    {{&machine_pin_irq_type},  0, GPIO_PIN0},
    {{&machine_pin_irq_type},  1, GPIO_PIN1},
    {{&machine_pin_irq_type},  2, GPIO_PIN2},
    {{&machine_pin_irq_type},  3, GPIO_PIN3},
    {{&machine_pin_irq_type},  4, GPIO_PIN4},
    {{&machine_pin_irq_type},  5, GPIO_PIN5},
    {{&machine_pin_irq_type},  6, GPIO_PIN6},
    {{&machine_pin_irq_type},  7, GPIO_PIN7},
};

STATIC mp_obj_t machine_pin_irq_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    machine_pin_irq_obj_t *self = self_in;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    machine_pin_isr_handler((void *)&machine_pin_obj[self->phys_port]);
    return mp_const_none;
}


STATIC const mp_obj_type_t machine_pin_irq_type = {
    { &mp_type_type },
    .name = MP_QSTR_IRQ,
    .call = machine_pin_irq_call,
    .locals_dict = (mp_obj_dict_t *)&machine_pin_irq_locals_dict,
};

/******************************************************************************/

void mp_hal_pin_input(mp_hal_pin_obj_t pin_id) {
    const machine_pin_obj_t *self = &machine_pin_obj[pin_id];
    GPIO_ChangeMode(self->id, GPIO_MODE_INPUT);
}

void mp_hal_pin_output(mp_hal_pin_obj_t pin_id) {
    const machine_pin_obj_t *self = &machine_pin_obj[pin_id];
    GPIO_ChangeMode(self->id, GPIO_MODE_OUTPUT);

}

int mp_hal_pin_read(mp_hal_pin_obj_t pin_id) {
    const machine_pin_obj_t *self = &machine_pin_obj[pin_id];
    GPIO_LEVEL level;
    GPIO_Get(self->id, &level);
    return (int) level;
}

void mp_hal_pin_write(mp_hal_pin_obj_t pin_id, int value) {
    const machine_pin_obj_t *self = &machine_pin_obj[pin_id];
    GPIO_Set(self->id, (GPIO_LEVEL) value);
}

mp_hal_pin_obj_t mp_hal_get_pin_obj(mp_obj_t pin_in) {
    if (mp_obj_get_type(pin_in) != &machine_pin_type) {
        mp_raise_ValueError("expecting a pin");
    }
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pin_in);
    return self->phys_port;
}
