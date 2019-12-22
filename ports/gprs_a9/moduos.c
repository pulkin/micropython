/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
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

#include "moduos.h"

#include <string.h>

#include "py/objstr.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "extmod/misc.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"
#include "genhdr/mpversion.h"

#include "rng.h"
#include "api_fs.h"

extern const mp_obj_type_t mp_fat_vfs_type;

void moduos_init0() {
    API_FS_ChangeDir("/");
}

STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_nodename,
    MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_nodename_obj, MICROPY_PY_SYS_PLATFORM);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, MICROPY_VERSION_STRING);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, MICROPY_HW_BOARD_NAME " with " MICROPY_HW_MCU_NAME);

STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    5,
    (mp_obj_t)&os_uname_info_sysname_obj,
    (mp_obj_t)&os_uname_info_nodename_obj,
    (mp_obj_t)&os_uname_info_release_obj,
    (mp_obj_t)&os_uname_info_version_obj,
    (mp_obj_t)&os_uname_info_machine_obj
);

STATIC mp_obj_t os_uname(void) {
    // ========================================
    // Platform information.
    // ========================================
    return (mp_obj_t)&os_uname_info_obj;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC mp_obj_t os_urandom(mp_obj_t num) {
    // ========================================
    // Random bytes.
    // Args:
    //     num (int): the number of bytes to generate;
    // ========================================
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    uint32_t r = 0;
    for (int i = 0; i < n; i++) {
        if ((i & 3) == 0) {
            r = rng_get(); // no hardware rng, soft impl
        }
        vstr.buf[i] = r;
        r >>= 8;
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_urandom_obj, os_urandom);

// -------------
// Native FS API
// -------------

typedef struct _native_vfs_ilistdir_it_t {
    mp_obj_base_t base;
    mp_fun_1_t iternext;
    Dir_t* folder;
} native_vfs_ilistdir_it_t;

mp_obj_t moduos_flashfs_mount(mp_obj_t arg1, mp_obj_t arg2) {
    // ========================================
    // Mounts the filesystem.
    // ========================================
    (void) arg1;
    (void) arg2;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(moduos_flashfs_mount_obj, moduos_flashfs_mount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_mount_static_class_obj, &moduos_flashfs_mount_obj);

mp_obj_t moduos_flashfs_umount() {
    // ========================================
    // Unmounts the filesystem.
    // ========================================
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(moduos_flashfs_umount_obj, moduos_flashfs_umount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_umount_static_class_obj, &moduos_flashfs_umount_obj);

STATIC mp_obj_t flashfs_ilistdir_it_iternext(mp_obj_t self_in) {
    // ========================================
    // next(ilistdir)
    // ========================================
    native_vfs_ilistdir_it_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->folder == NULL)
        // stopped iterating already
        return MP_OBJ_STOP_ITERATION;

    const Dirent_t* entry = NULL;
    while ((entry = API_FS_ReadDir(self->folder))) {
        // output the entry
        mp_obj_t tuple[3] = {
            mp_obj_new_str(entry->d_name, strlen(entry->d_name)),
            mp_obj_new_int(entry->d_type << 12),
            mp_obj_new_int(entry->d_ino),
        };
        return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
    }

    // finallize the iterator
    API_FS_CloseDir(self->folder);
    self->folder = NULL;
    return MP_OBJ_STOP_ITERATION;
}

mp_obj_t moduos_flashfs_ilistdir(mp_obj_t path_in) {
    // ========================================
    // Iterates a folder.
    // Args:
    //     name (str): folder to list;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    Dir_t* dir;
    if (strlen(path))
        dir = API_FS_OpenDir(path);
    else
        dir = API_FS_OpenDir(".");
    if (!dir)
        mp_raise_OSError(MP_EIO);

    native_vfs_ilistdir_it_t *iter = m_new_obj(native_vfs_ilistdir_it_t);
    iter->base.type = &mp_type_polymorph_iter;
    iter->iternext = flashfs_ilistdir_it_iternext;
    iter->folder = dir;
    return MP_OBJ_FROM_PTR(iter);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_ilistdir_obj, moduos_flashfs_ilistdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_ilistdir_static_class_obj, &moduos_flashfs_ilistdir_obj);

mp_obj_t moduos_flashfs_mkdir(mp_obj_t path_in) {
    // ========================================
    // Creates a folder.
    // Args:
    //     name (str): folder to create;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    int32_t ret = API_FS_Mkdir(path, 0);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_mkdir_obj, moduos_flashfs_mkdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_mkdir_static_class_obj, &moduos_flashfs_mkdir_obj);

mp_obj_t moduos_flashfs_remove(mp_obj_t path_in) {
    // ========================================
    // Removes a file.
    // Args:
    //     name (str): file to remove;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    int32_t ret = API_FS_Delete(path);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_remove_obj, moduos_flashfs_remove);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_remove_static_class_obj, &moduos_flashfs_remove_obj);

mp_obj_t moduos_flashfs_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    // ========================================
    // Renames a file.
    // Args:
    //     name (str): file to rename;
    //     new (str): the new name;
    // ========================================
    const char* path_old = mp_obj_str_get_str(old_path_in);
    const char* path_new = mp_obj_str_get_str(new_path_in);
    int32_t ret = API_FS_Rename(path_old, path_new);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(moduos_flashfs_rename_obj, moduos_flashfs_rename);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_rename_static_class_obj, &moduos_flashfs_rename_obj);

mp_obj_t moduos_flashfs_rmdir(mp_obj_t path_in) {
    // ========================================
    // Removes a folder.
    // Args:
    //     name (str): folder to remove;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    int32_t ret = API_FS_Rmdir(path);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_rmdir_obj, moduos_flashfs_rmdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_rmdir_static_class_obj, &moduos_flashfs_rmdir_obj);

mp_obj_t moduos_flashfs_chdir(mp_obj_t path_in) {
    // ========================================
    // Changes a folder.
    // Args:
    //     name (str): folder to change to;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    int32_t ret = API_FS_ChangeDir(path);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_chdir_obj, moduos_flashfs_chdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_chdir_static_class_obj, &moduos_flashfs_chdir_obj);

mp_obj_t moduos_flashfs_getcwd(void) {
    // ========================================
    // Retrieves current folder.
    // ========================================
    char dir[256];
    uint32_t ret = API_FS_GetCurDir(sizeof(dir), dir);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_obj_new_str(dir, strlen(dir));
}

MP_DEFINE_CONST_FUN_OBJ_0(moduos_flashfs_getcwd_obj, moduos_flashfs_getcwd);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_getcwd_static_class_obj, &moduos_flashfs_getcwd_obj);

mp_obj_t moduos_flashfs_stat(mp_obj_t path_in) {
    // ========================================
    // Stats the file system.
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    API_FS_INFO info;
    int32_t ret = API_FS_GetFSInfo(path, &info);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    mp_obj_t tuple[2] = {
        mp_obj_new_int(info.totalSize),
        mp_obj_new_int(info.usedSize),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_stat_obj, moduos_flashfs_stat);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_stat_static_class_obj, &moduos_flashfs_stat_obj);

mp_obj_t moduos_flashfs_statvfs(mp_obj_t path_in) {
    // ========================================
    // Stats the file system.
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    API_FS_INFO info;
    int32_t ret = API_FS_GetFSInfo(path, &info);
    if (ret != 0)
        mp_raise_OSError(MP_EIO);
    mp_obj_t tuple[2] = {
        mp_obj_new_int(info.totalSize),
        mp_obj_new_int(info.usedSize),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_flashfs_statvfs_obj, moduos_flashfs_statvfs);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_flashfs_statvfs_static_class_obj, &moduos_flashfs_statvfs_obj);

STATIC const mp_rom_map_elem_t flashfs_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&moduos_flashfs_mount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&moduos_flashfs_umount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&moduos_flashfs_ilistdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&moduos_flashfs_mkdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&moduos_flashfs_remove_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&moduos_flashfs_rename_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&moduos_flashfs_rmdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&moduos_flashfs_chdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&moduos_flashfs_getcwd_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&moduos_flashfs_stat_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&moduos_flashfs_statvfs_static_class_obj) },
};

STATIC MP_DEFINE_CONST_DICT(flashfs_locals_dict, flashfs_locals_dict_table);

STATIC const mp_obj_type_t moduos_flashfs_type = {
    { &mp_type_type },
    .name = MP_QSTR_flashfs,
    .locals_dict = (mp_obj_dict_t*)&flashfs_locals_dict,
};

#if MICROPY_PY_OS_DUPTERM
STATIC mp_obj_t os_dupterm_notify(mp_obj_t obj_in) {
    (void) obj_in;
    for (;;) {
        int c = mp_uos_dupterm_rx_chr();
        if (c < 0) {
            break;
        }
        ringbuf_put(&stdin_ringbuf, c);
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_dupterm_notify_obj, os_dupterm_notify);
#endif

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_flashfs), (mp_obj_t)MP_ROM_PTR(&moduos_flashfs_type) },

    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },
    { MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&os_urandom_obj) },
    #if MICROPY_PY_OS_DUPTERM
    { MP_ROM_QSTR(MP_QSTR_dupterm), MP_ROM_PTR(&mp_uos_dupterm_obj) },
    { MP_ROM_QSTR(MP_QSTR_dupterm_notify), MP_ROM_PTR(&os_dupterm_notify_obj) },
    #endif
    #if MICROPY_VFS
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&mp_vfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&mp_vfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mp_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&mp_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&mp_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&mp_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&mp_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&mp_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&mp_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&mp_vfs_statvfs_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&mp_vfs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&mp_vfs_umount_obj) },
    #if MICROPY_VFS_FAT
    { MP_ROM_QSTR(MP_QSTR_VfsFat), MP_ROM_PTR(&mp_fat_vfs_type) },
    #endif
    #endif
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t uos_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
