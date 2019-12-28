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
#include "moduos.h"

#include <string.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "genhdr/mpversion.h"

#include "api_fs.h"

typedef struct _internal_flash_file_obj_t {
    mp_obj_base_t base;
    int fd;
} internal_flash_file_obj_t;

STATIC void internal_flash_file_obj_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    mp_printf(print, "<io.%s %p>", mp_obj_get_type_str(self_in), MP_OBJ_TO_PTR(self_in));
}

STATIC mp_uint_t internal_flash_file_obj_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    // ========================================
    // f.read()
    // ========================================
    internal_flash_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ret = API_FS_Read(self->fd, buf, size);
    if (ret < 0) {
        *errcode = translate_io_errno(ret);
        return MP_STREAM_ERROR;
    }
    return (mp_uint_t) ret;
}

STATIC mp_uint_t internal_flash_file_obj_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    // ========================================
    // f.write(s)
    // ========================================
    internal_flash_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int32_t ret = API_FS_Write(self->fd, (uint8_t*) buf, size);
    if (ret < 0) {
        *errcode = translate_io_errno(ret);
        return MP_STREAM_ERROR;
    }
    if (ret != size) {
        *errcode = MP_ENOSPC;
        return MP_STREAM_ERROR;
    }
    return (mp_uint_t) ret;
}

STATIC mp_obj_t file_obj___exit__(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return mp_stream_close(args[0]);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(file_obj___exit___obj, 4, 4, file_obj___exit__);

STATIC mp_uint_t internal_flash_file_obj_ioctl(mp_obj_t o_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    // ========================================
    // IO stream control: f.seek(), f.flush(), f.close()
    // ========================================
    internal_flash_file_obj_t *self = MP_OBJ_TO_PTR(o_in);

    if (request == MP_STREAM_SEEK) {
        struct mp_stream_seek_t *s = (struct mp_stream_seek_t*) arg;
        int64_t ret = 0;

        switch (s->whence) {
            case 0: // SEEK_SET
                ret = API_FS_Seek(self->fd, s->offset, FS_SEEK_SET);
                break;

            case 1: // SEEK_CUR
                ret = API_FS_Seek(self->fd, s->offset, FS_SEEK_CUR);
                break;

            case 2: // SEEK_END
                ret = API_FS_Seek(self->fd, s->offset, FS_SEEK_END);
                break;
        }

        if (ret < 0) {
            *errcode = translate_io_errno((int) ret);
            return MP_STREAM_ERROR;
        }

        s->offset = ret;
        return 0;

    } else if (request == MP_STREAM_FLUSH) {
        uint32_t ret = API_FS_Flush(self->fd);
        if (ret < 0) {
            *errcode = translate_io_errno(ret);
            return MP_STREAM_ERROR;
        }
        return 0;

    } else if (request == MP_STREAM_CLOSE) {
        // if fs==NULL then the file is closed and in that case this method is a no-op
        if (self->fd > 0) {
            int32_t ret = API_FS_Close(self->fd);
            if (ret < 0) {
                *errcode = translate_io_errno(ret);
                return MP_STREAM_ERROR;
            } else
                self->fd = 0;
        }
        return 0;

    } else {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
    return MP_STREAM_ERROR;
}

STATIC const mp_arg_t file_open_args[] = {
    { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r)} },
    { MP_QSTR_encoding, MP_ARG_OBJ | MP_ARG_KW_ONLY, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
};

#define FILE_OPEN_NUM_ARGS MP_ARRAY_SIZE(file_open_args)

STATIC mp_obj_t internal_flash_file_open(const char* file_name, const mp_obj_type_t *type, mp_arg_val_t *args) {
    // ========================================
    // open(...)
    // ========================================
    const char *mode_s = mp_obj_str_get_str(args[1].u_obj);
    uint32_t mode = 0;
    while (*mode_s) {
        switch (*mode_s++) {
            case 'r':
                mode |= FS_O_RDONLY;
                break;
            case 'w':
                mode |= FS_O_WRONLY | FS_O_CREAT | FS_O_TRUNC;
                break;
            case 'a':
                mode |= FS_O_WRONLY | FS_O_CREAT | FS_O_APPEND;
                break;
            case '+':
                mode |= FS_O_RDWR;
                break;
            #if MICROPY_PY_IO_FILEIO
            case 'b':
                type = &mp_type_internal_flash_fileio;
                break;
            case 't':
                type = &mp_type_internal_flash_textio;
                break;
            #endif
            default:
                mp_raise_ValueError("Mode must be one or more of 'rwabt+'");
        }
    }
    internal_flash_file_obj_t *o = m_new_obj_with_finaliser(internal_flash_file_obj_t);
    o->base.type = type;

    int32_t fd = maybe_raise_FSError(API_FS_Open(file_name, mode, 0));
    if (fd <= 0)
        mp_raise_OSError(MP_ENOENT);
    o->fd = fd;
    return MP_OBJ_FROM_PTR(o);
}

STATIC mp_obj_t internal_flash_file_obj_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_val_t arg_vals[FILE_OPEN_NUM_ARGS];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, FILE_OPEN_NUM_ARGS, file_open_args, arg_vals);
    return internal_flash_file_open(NULL, type, arg_vals);
}

STATIC const mp_rom_map_elem_t vfs_fat_rawfile_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&file_obj___exit___obj) },
};

STATIC MP_DEFINE_CONST_DICT(vfs_fat_rawfile_locals_dict, vfs_fat_rawfile_locals_dict_table);

#if MICROPY_PY_IO_FILEIO
STATIC const mp_stream_p_t internal_flash_fileio_stream_p = {
    .read = internal_flash_file_obj_read,
    .write = internal_flash_file_obj_write,
    .ioctl = internal_flash_file_obj_ioctl,
};

const mp_obj_type_t mp_type_internal_flash_fileio = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .print = internal_flash_file_obj_print,
    .make_new = internal_flash_file_obj_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &internal_flash_fileio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&vfs_fat_rawfile_locals_dict,
};
#endif

STATIC const mp_stream_p_t internal_flash_textio_stream_p = {
    .read = internal_flash_file_obj_read,
    .write = internal_flash_file_obj_write,
    .ioctl = internal_flash_file_obj_ioctl,
    .is_text = true,
};

const mp_obj_type_t mp_type_internal_flash_textio = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .print = internal_flash_file_obj_print,
    .make_new = internal_flash_file_obj_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &internal_flash_textio_stream_p,
    .locals_dict = (mp_obj_dict_t*)&vfs_fat_rawfile_locals_dict,
};

mp_obj_t internal_flash_open(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // ========================================
    // open(...) wrapper
    // ========================================
    // TODO: binary mode
    enum { ARG_file, ARG_mode, ARG_encoding };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_QSTR(MP_QSTR_r)} },
        { MP_QSTR_buffering, MP_ARG_INT, {.u_int = -1} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    const char* file_name = mp_obj_str_get_str(args[ARG_file].u_obj);    
    const mp_obj_type_t* type = &mp_type_internal_flash_textio;
    return internal_flash_file_open(file_name, type, args);
}

MP_DEFINE_CONST_FUN_OBJ_KW(internal_flash_open_obj, 1, internal_flash_open);

