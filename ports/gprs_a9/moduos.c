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
#include "api_hal_spi.h"

#define PATH_IS_FILE 1
#define PATH_IS_DIR 2

extern const mp_obj_type_t mp_fat_vfs_type;

mp_int_t translate_io_errno(int err) {
    switch (err) {
        case ERR_FS_IS_DIRECTORY:
            return MP_EISDIR;
        case ERR_FS_NOT_DIRECTORY:
            return MP_ENOTDIR;
        case ERR_FS_NO_DIR_ENTRY:
            return MP_ENOENT;
        case ERR_FS_OPERATION_NOT_GRANTED:
            return MP_EIO; // TODO: translate better?
        case ERR_FS_DIR_NOT_EMPTY:
            return MP_EBUSY; // TODO: translate better?
        case ERR_FS_FDS_MAX:
            return MP_EIO; // TODO: translate better?
        case ERR_FS_PROCESS_FILE_MAX:
            return MP_EMFILE;
        case ERR_FS_FILE_EXIST:
            return MP_EEXIST;
        case ERR_FS_NO_BASENAME:
            return MP_ENOENT;
        case ERR_FS_BAD_FD:
            return MP_EBADF;
        case ERR_FS_NO_MORE_FILES:
            return MP_ENOENT; // TODO: translate better?
        case ERR_FS_HAS_MOUNTED:
            return MP_EEXIST; // TODO: translate better?
        case ERR_FS_MOUNTED_FS_MAX:
            return MP_EMFILE;
        case ERR_FS_UNKNOWN_FILESYSTEM:
            return MP_EIO;
        case ERR_FS_INVALID_DIR_ENTRY:
            return MP_EPERM; // TODO: translate better?
        case ERR_FS_INVALID_PARAMETER:
            return MP_EINVAL;
        case ERR_FS_NOT_SUPPORT:
            return MP_EIO;
        case ERR_FS_UNMOUNT_FAILED:
            return MP_EBUSY;
        case ERR_FS_NO_MORE_MEMORY:
            return MP_ENOMEM;
        case ERR_FS_DEVICE_NOT_REGISTER:
            return MP_ENXIO;
        case ERR_FS_DISK_FULL:
            return MP_ENOSPC;
        case ERR_FS_NOT_FORMAT:
            return MP_EIO;
        case ERR_FS_HAS_FORMATED:
            return MP_EIO;
        case ERR_FS_NOT_FIND_SB:
            return MP_EIO;
        case ERR_FS_DEVICE_BUSY:
            return MP_EBUSY;
        case ERR_FS_OPEN_DEV_FAILED:
            return MP_EIO;
        case ERR_FS_ROOT_FULL:
            return MP_EIO; // TODO: wtf is this?
        case ERR_FS_ACCESS_REG_FAILED:
            return MP_EIO; // TODO: poor translation here.
        case ERR_FS_PATHNAME_PARSE_FAILED:
            return MP_EINVAL;
        case ERR_FS_READ_DIR_FAILED:
            return MP_EIO;
        case ERR_FS_MOUNT_READ_ROOT_INODE_FAILED:
            return MP_EIO;
        case ERR_FS_INVALID_DEV_NUMBER:
            return MP_ENODEV;
        case ERR_FS_RENAME_DIFF_PATH:
            return MP_EISDIR;
        case ERR_FS_FORMAT_MOUNTING_DEVICE:
            return MP_EBUSY;
        case ERR_FS_DATA_DESTROY:
            return MP_EIO;
        case ERR_FS_READ_SECTOR_FAILED:
            return MP_EIO;
        case ERR_FS_WRITE_SECTOR_FAILED:
            return MP_EIO;
        case ERR_FS_READ_FILE_EXCEED:
            return MP_EIO;
        case ERR_FS_WRITE_FILE_EXCEED:
            return MP_EIO;
        case ERR_FS_FILE_TOO_MORE:
            return MP_EINVAL;
        case ERR_FS_FILE_NOT_EXIST:
            return MP_ENOENT;
        case ERR_FS_DEVICE_DIFF:
            return MP_EACCES;
        case ERR_FS_GET_DEV_INFO_FAILED:
            return MP_EIO;
        case ERR_FS_NO_MORE_SB_ITEM:
            return MP_EIO;
        case ERR_FS_NOT_MOUNT:
            return MP_ENXIO;
        case ERR_FS_NAME_BUFFER_TOO_SHORT:
            return MP_EINVAL;
        case ERR_FS_NOT_REGULAR:
            return MP_EACCES;
        case ERR_FS_VOLLAB_IS_NULL:
            return MP_EACCES;
        default:
            return 0;
    }
}

int maybe_raise_FSError(int errno) {
    if (errno < -4200000)
        mp_raise_OSError(translate_io_errno(errno));
    if (errno < 0)
        mp_raise_OSError(MP_EIO);
    return errno;
}

char path_exists(const char* path) {
    Dir_t* folder = NULL;
    if ((folder = API_FS_OpenDir(path))) {
        API_FS_CloseDir(folder);
        return PATH_IS_DIR;
    }
    int32_t fd;
    if ((fd = API_FS_Open(path, FS_O_RDONLY, 0)) >= 0) {
        API_FS_Close(fd);
        return PATH_IS_FILE;
    }
    return 0;
}

void ensure_not_a_directory(const char* path) {
    if (path_exists(path) & PATH_IS_DIR)
        mp_raise_OSError(MP_EISDIR);
}

void ensure_not_a_file(const char* path) {
    if (path_exists(path) & PATH_IS_FILE)
        mp_raise_OSError(MP_EEXIST);
}

void ensure_is_dir(const char* path) {
    int x = path_exists(path);
    if (!x)
        mp_raise_OSError(MP_ENOENT);
    if (!(x & PATH_IS_DIR))
        mp_raise_OSError(MP_ENOTDIR);
}

void ensure_is_file(const char* path) {
    int x = path_exists(path);
    if (!x)
        mp_raise_OSError(MP_ENOENT);
    if (!(x & PATH_IS_FILE))
        mp_raise_OSError(MP_EISDIR);
}

void ensure_not_exists(const char* path) {
    if (path_exists(path))
        mp_raise_OSError(MP_EEXIST);
}

void ensure_exists(const char* path) {
    if (!path_exists(path))
        mp_raise_OSError(MP_ENOENT);
}

void moduos_init0() {
    int errno;
    if ((errno = API_FS_ChangeDir("/")))
        mp_printf(&mp_plat_print, "Warning: moduos_init0 failed to chdir to /, error code: %d\n", translate_io_errno(errno));
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

mp_obj_t moduos_internal_flash_mount(mp_obj_t arg1, mp_obj_t arg2) {
    // ========================================
    // Mounts the filesystem.
    // ========================================
    (void) arg1;
    (void) arg2;
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(moduos_internal_flash_mount_obj, moduos_internal_flash_mount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_mount_static_class_obj, &moduos_internal_flash_mount_obj);

mp_obj_t moduos_internal_flash_umount() {
    // ========================================
    // Unmounts the filesystem.
    // ========================================
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(moduos_internal_flash_umount_obj, moduos_internal_flash_umount);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_umount_static_class_obj, &moduos_internal_flash_umount_obj);

STATIC mp_obj_t internal_flash_ilistdir_it_iternext(mp_obj_t self_in) {
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
    maybe_raise_FSError(API_FS_CloseDir(self->folder));
    self->folder = NULL;
    return MP_OBJ_STOP_ITERATION;
}

mp_obj_t moduos_internal_flash_ilistdir(mp_obj_t path_in) {
    // ========================================
    // Iterates a folder.
    // Args:
    //     name (str): folder to list;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    ensure_is_dir(path);
    Dir_t* dir = API_FS_OpenDir(path);
    native_vfs_ilistdir_it_t *iter = m_new_obj(native_vfs_ilistdir_it_t);
    iter->base.type = &mp_type_polymorph_iter;
    iter->iternext = internal_flash_ilistdir_it_iternext;
    iter->folder = dir;
    return MP_OBJ_FROM_PTR(iter);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_ilistdir_obj, moduos_internal_flash_ilistdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_ilistdir_static_class_obj, &moduos_internal_flash_ilistdir_obj);

mp_obj_t moduos_internal_flash_mkdir(mp_obj_t path_in) {
    // ========================================
    // Creates a folder.
    // Args:
    //     name (str): folder to create;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    ensure_not_exists(path);
    maybe_raise_FSError(API_FS_Mkdir(path, 0));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_mkdir_obj, moduos_internal_flash_mkdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_mkdir_static_class_obj, &moduos_internal_flash_mkdir_obj);

mp_obj_t moduos_internal_flash_remove(mp_obj_t path_in) {
    // ========================================
    // Removes a file.
    // Args:
    //     name (str): file to remove;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    ensure_is_file(path);
    maybe_raise_FSError(API_FS_Delete(path));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_remove_obj, moduos_internal_flash_remove);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_remove_static_class_obj, &moduos_internal_flash_remove_obj);

mp_obj_t moduos_internal_flash_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    // ========================================
    // Renames a file.
    // Args:
    //     name (str): file to rename;
    //     new (str): the new name;
    // ========================================
    const char* path_old = mp_obj_str_get_str(old_path_in);
    const char* path_new = mp_obj_str_get_str(new_path_in);
    ensure_exists(path_old);
    maybe_raise_FSError(API_FS_Rename(path_old, path_new));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_2(moduos_internal_flash_rename_obj, moduos_internal_flash_rename);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_rename_static_class_obj, &moduos_internal_flash_rename_obj);

mp_obj_t moduos_internal_flash_rmdir(mp_obj_t path_in) {
    // ========================================
    // Removes a folder.
    // Args:
    //     name (str): folder to remove;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    ensure_is_dir(path);
    maybe_raise_FSError(API_FS_Rmdir(path));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_rmdir_obj, moduos_internal_flash_rmdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_rmdir_static_class_obj, &moduos_internal_flash_rmdir_obj);

mp_obj_t moduos_internal_flash_chdir(mp_obj_t path_in) {
    // ========================================
    // Changes a folder.
    // Args:
    //     name (str): folder to change to;
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    ensure_is_dir(path);
    maybe_raise_FSError(API_FS_ChangeDir(path));
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_chdir_obj, moduos_internal_flash_chdir);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_chdir_static_class_obj, &moduos_internal_flash_chdir_obj);

mp_obj_t moduos_internal_flash_getcwd(void) {
    // ========================================
    // Retrieves current folder.
    // ========================================
    char dir[256];
    maybe_raise_FSError(API_FS_GetCurDir(sizeof(dir), dir));
    return mp_obj_new_str(dir, strlen(dir));
}

MP_DEFINE_CONST_FUN_OBJ_0(moduos_internal_flash_getcwd_obj, moduos_internal_flash_getcwd);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_getcwd_static_class_obj, &moduos_internal_flash_getcwd_obj);

mp_obj_t moduos_internal_flash_stat(mp_obj_t path_in) {
    // ========================================
    // Stats the file system.
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    // check if folder
    int32_t size = 0;
    Dir_t* folder = NULL;
    mp_int_t mode = 0;
    int32_t fd = NULL;

    if ((folder = API_FS_OpenDir(path))) {
        API_FS_CloseDir(folder);
        mode = MP_S_IFDIR;

    } else if ((fd = API_FS_Open(path, FS_O_RDONLY, 0)) >= 0) {
        API_FS_Close(fd);
        size = API_FS_GetFileSize(fd);
        mode = MP_S_IFREG;

    } else
        mp_raise_OSError(MP_ENOENT);

    mp_obj_t tuple[10] = {
        MP_OBJ_NEW_SMALL_INT(mode),
        MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_NEW_SMALL_INT(0),
        mp_obj_new_int(size),
        MP_OBJ_NEW_SMALL_INT(0x2821),
        MP_OBJ_NEW_SMALL_INT(0x2821),
        MP_OBJ_NEW_SMALL_INT(0x2821),
    };
    return mp_obj_new_tuple(10, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_stat_obj, moduos_internal_flash_stat);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_stat_static_class_obj, &moduos_internal_flash_stat_obj);

mp_obj_t moduos_internal_flash_statvfs(mp_obj_t path_in) {
    // ========================================
    // Stats the file system.
    // ========================================
    const char* path = mp_obj_str_get_str(path_in);
    API_FS_INFO info;
    maybe_raise_FSError(API_FS_GetFSInfo(path, &info));
    mp_obj_t tuple[2] = {
        mp_obj_new_int(info.totalSize),
        mp_obj_new_int(info.usedSize),
    };
    return mp_obj_new_tuple(2, tuple);
}

MP_DEFINE_CONST_FUN_OBJ_1(moduos_internal_flash_statvfs_obj, moduos_internal_flash_statvfs);
MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_statvfs_static_class_obj, &moduos_internal_flash_statvfs_obj);

MP_DEFINE_CONST_STATICMETHOD_OBJ(moduos_internal_flash_open_static_class_obj, &internal_flash_open_obj);

STATIC const mp_rom_map_elem_t internal_flash_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&moduos_internal_flash_open_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_mount), MP_ROM_PTR(&moduos_internal_flash_mount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount), MP_ROM_PTR(&moduos_internal_flash_umount_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir), MP_ROM_PTR(&moduos_internal_flash_ilistdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&moduos_internal_flash_mkdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&moduos_internal_flash_remove_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&moduos_internal_flash_rename_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&moduos_internal_flash_rmdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&moduos_internal_flash_chdir_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&moduos_internal_flash_getcwd_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&moduos_internal_flash_stat_static_class_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&moduos_internal_flash_statvfs_static_class_obj) },
};

STATIC MP_DEFINE_CONST_DICT(internal_flash_locals_dict, internal_flash_locals_dict_table);

STATIC const mp_obj_type_t moduos_internal_flash_type = {
    { &mp_type_type },
    .name = MP_QSTR_internal_flash,
    .locals_dict = (mp_obj_dict_t*)&internal_flash_locals_dict,
};

#if MICROPY_PY_OS_DUPTERM
/* STATIC mp_obj_t os_dupterm_notify(mp_obj_t obj_in) {
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

STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_dupterm_notify_obj, os_dupterm_notify);*/
#endif

STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_uos) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_internal_flash), (mp_obj_t)MP_ROM_PTR(&moduos_internal_flash_type) },

    { MP_ROM_QSTR(MP_QSTR_uname), MP_ROM_PTR(&os_uname_obj) },
    { MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&os_urandom_obj) },
    #if MICROPY_PY_OS_DUPTERM
    { MP_ROM_QSTR(MP_QSTR_dupterm), MP_ROM_PTR(&mp_uos_dupterm_obj) },
    // { MP_ROM_QSTR(MP_QSTR_dupterm_notify), MP_ROM_PTR(&mp_os_dupterm_notify_obj) },
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
