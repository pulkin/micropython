/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 nk2IsHere
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

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_hal_i2c.h"

// ----------
// Exceptions
// ----------

MP_DEFINE_EXCEPTION(I2CError, OSError)

NORETURN void mp_raise_I2CError(const char *msg) {
    mp_raise_msg(&mp_type_I2CError, msg);
}

I2C_ID_t modi2c_private_get_I2C_ID(uint8_t _id) {
    I2C_ID_t id;
    switch(_id) {
        case 2:
            id = I2C2;
            break;
        case 3:
            id = I2C3;
            break;  
        default:
            id = I2C1;
    }
    return id;
}

I2C_FREQ_t modi2c_private_get_I2C_FREQ(uint8_t _frequency) {
    I2C_FREQ_t frequency;
    switch(_frequency) {
        case 400:
            frequency = I2C_FREQ_400K;
            break;
        default:
            frequency = I2C_FREQ_100K;
    }
    return frequency;
}

void modi2c_private_throw_I2C_Error(I2C_Error_t error) {
    switch(error) {
        case I2C_ERROR_RESOURCE_RESET:
            mp_raise_I2CError("A resource reset is required");
            break;
        case I2C_ERROR_RESOURCE_BUSY:
            mp_raise_I2CError("An attempt to access a busy resource failed");
            break;
        case I2C_ERROR_RESOURCE_TIMEOUT:
            mp_raise_I2CError("Timeout while trying to access the resource");
            break;
        case I2C_ERROR_RESOURCE_NOT_ENABLED:
            mp_raise_I2CError("An attempt to access a resource that is not enabled");
            break;
        case I2C_ERROR_BAD_PARAMETER:
            mp_raise_I2CError("Invalid parameter");
            break;
        case I2C_ERROR_COMMUNICATION_FAILED:
            mp_raise_I2CError("Communication failure");
            break;
    }
}

// ---------
// Methods
// ---------

STATIC mp_obj_t modi2c_init(size_t n_args, const mp_obj_t *arg) {
    if (n_args != 2) {
        mp_raise_I2CError("I2C id and I2C frequency are required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        I2C_FREQ_t frequency = modi2c_private_get_I2C_FREQ(mp_obj_get_int(arg[1]));
        I2C_Config_t config;

        config.freq = frequency;
        if(!I2C_Init(id, config)) {
            mp_raise_I2CError("I2C init failure");
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_init_obj, 0, 2, modi2c_init);

STATIC mp_obj_t modi2c_close(size_t n_args, const mp_obj_t *arg) {
    if (n_args != 1) {
        mp_raise_I2CError("I2C id is required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        if(!I2C_Close(id)) {
            mp_raise_I2CError("I2C close failure");
        }
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_close_obj, 0, 1, modi2c_close);

STATIC mp_obj_t modi2c_receive(size_t n_args, const mp_obj_t *arg) {
    if (n_args < 3) {
        mp_raise_I2CError("I2C id, slave address, data length, [timeout] are required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        uint16_t slave_address = mp_obj_get_int(arg[1]);
        uint16_t length = mp_obj_get_int(arg[2]);
        uint16_t timeout = I2C_DEFAULT_TIME_OUT;
        if(n_args == 4)
            timeout = mp_obj_get_int(arg[3]);

        vstr_t vstr;
        vstr_init_len(&vstr, length);

        I2C_Error_t error = I2C_Receive(id, slave_address, (uint8_t*)vstr.buf, vstr.len, timeout);
        modi2c_private_throw_I2C_Error(error);

        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_receive_obj, 0, 4, modi2c_receive);

STATIC mp_obj_t modi2c_transmit(size_t n_args, const mp_obj_t *arg) {
    if (n_args < 3) {
        mp_raise_I2CError("I2C id, slave address, data, [timeout] are required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        uint16_t slave_address = mp_obj_get_int(arg[1]);
        mp_buffer_info_t data_buffer;
        mp_get_buffer_raise(arg[2], &data_buffer, MP_BUFFER_READ);
        uint16_t timeout = I2C_DEFAULT_TIME_OUT;
        if(n_args == 4)
            timeout = mp_obj_get_int(arg[3]);

        I2C_Error_t error = I2C_Transmit(id, slave_address, (uint8_t*)data_buffer.buf, data_buffer.len, timeout);
        modi2c_private_throw_I2C_Error(error);

        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_transmit_obj, 0, 4, modi2c_transmit);

STATIC mp_obj_t modi2c_mem_receive(size_t n_args, const mp_obj_t *arg) {
    if (n_args < 5) {
        mp_raise_I2CError("I2C id, slave address, memory address, memory size, memory length, [timeout] are required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        uint16_t slave_address = mp_obj_get_int(arg[1]);
        uint32_t memory_address = mp_obj_get_int(arg[2]);
        uint8_t memory_size = mp_obj_get_int(arg[3]);
        uint16_t length = mp_obj_get_int(arg[4]);
        uint16_t timeout = I2C_DEFAULT_TIME_OUT;
        if(n_args == 6)
            timeout = mp_obj_get_int(arg[5]);

        vstr_t vstr;
        vstr_init_len(&vstr, length);

        I2C_Error_t error = I2C_ReadMem(id, slave_address, memory_address, memory_size, (uint8_t*)vstr.buf, vstr.len, timeout);
        modi2c_private_throw_I2C_Error(error);

        return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_mem_receive_obj, 0, 6, modi2c_mem_receive);

STATIC mp_obj_t modi2c_mem_transmit(size_t n_args, const mp_obj_t *arg) {
    if (n_args < 6) {
        mp_raise_I2CError("I2C id, slave address, memory address, memory size, data, [timeout] are required");
    } else {
        I2C_ID_t id = modi2c_private_get_I2C_ID(mp_obj_get_int(arg[0]));
        uint16_t slave_address = mp_obj_get_int(arg[1]);
        uint32_t memory_address = mp_obj_get_int(arg[2]);
        uint8_t memory_size = mp_obj_get_int(arg[3]);
        mp_buffer_info_t data_buffer;
        mp_get_buffer_raise(arg[4], &data_buffer, MP_BUFFER_READ);
        uint16_t timeout = I2C_DEFAULT_TIME_OUT;
        if(n_args == 6)
            timeout = mp_obj_get_int(arg[5]);

        I2C_Error_t error = I2C_WriteMem(id, slave_address, memory_address, memory_size, (uint8_t*)data_buffer.buf, data_buffer.len, timeout);
        modi2c_private_throw_I2C_Error(error);

        return mp_const_none;
    }
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modi2c_mem_transmit_obj, 0, 6, modi2c_mem_transmit);

// -------
// Modules
// -------

STATIC const mp_map_elem_t i2c_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_i2c) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_I2CError), (mp_obj_t)MP_ROM_PTR(&mp_type_I2CError) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_init), (mp_obj_t)&modi2c_init_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_close), (mp_obj_t)&modi2c_close_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_receive), (mp_obj_t)&modi2c_receive_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_transmit), (mp_obj_t)&modi2c_transmit_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mem_receive), (mp_obj_t)&modi2c_mem_receive_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_mem_transmit), (mp_obj_t)&modi2c_mem_transmit_obj },
};

STATIC MP_DEFINE_CONST_DICT (mp_module_i2c_globals, i2c_globals_table);

const mp_obj_module_t i2c_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_i2c_globals,
};
