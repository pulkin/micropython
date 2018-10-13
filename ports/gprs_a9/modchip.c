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

#include <stdio.h>


#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "api_hal_flash.h"


STATIC mp_obj_t chip_flash_read(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_WRITE);
    bool ret;
    Trace(1,"flash read offset:%d,0x%x",offset,offset);
    uint32_t addr = (uint32_t)HAL_SPI_FLASH_UNCACHE_ADDRESS(offset);
    Trace(1,"flash read:0x%x",addr);
    memcpy(bufinfo.buf, (void *)addr, bufinfo.len);
    ret = true;
    if (!ret) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(chip_flash_read_obj, chip_flash_read);

STATIC mp_obj_t chip_flash_write(mp_obj_t offset_in, mp_obj_t buf_in) {
    mp_int_t offset = mp_obj_get_int(offset_in);
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf_in, &bufinfo, MP_BUFFER_READ);
    bool ret = hal_SpiFlashWrite(offset,bufinfo.buf, bufinfo.len);
    Trace(1,"flash write:%d",ret);
    if (!ret) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(chip_flash_write_obj, chip_flash_write);

STATIC mp_obj_t chip_flash_erase(mp_obj_t sector_in) {
    mp_int_t sector = mp_obj_get_int(sector_in);
    bool ret = hal_SpiFlashErase(sector*4096,4096);
    Trace(1,"flash erase 0x%x, ret:%d",sector*4096,ret);
    if (!ret) {
        mp_raise_OSError(MP_EIO);
    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(chip_flash_erase_obj, chip_flash_erase);

STATIC mp_obj_t chip_flash_size(void) {
    return mp_obj_new_int_from_uint(hal_SpiFlashGetSize());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(chip_flash_size_obj, chip_flash_size);

STATIC mp_obj_t chip_flash_user_start(void) {
    return MP_OBJ_NEW_SMALL_INT(0x390000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(chip_flash_user_start_obj, chip_flash_user_start);

STATIC const mp_rom_map_elem_t chip_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_chip) },

    { MP_ROM_QSTR(MP_QSTR_flash_read), MP_ROM_PTR(&chip_flash_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_write), MP_ROM_PTR(&chip_flash_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_erase), MP_ROM_PTR(&chip_flash_erase_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_size), MP_ROM_PTR(&chip_flash_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_flash_user_start), MP_ROM_PTR(&chip_flash_user_start_obj) },
};

STATIC MP_DEFINE_CONST_DICT(chip_module_globals, chip_module_globals_table);

const mp_obj_module_t chip_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&chip_module_globals,
};

