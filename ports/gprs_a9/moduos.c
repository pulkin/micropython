/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Josef Gajdusek
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
    return (mp_obj_t)&os_uname_info_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);

STATIC mp_obj_t os_urandom(mp_obj_t num) {
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

//////////////// fs ////////////////////////////////////////////////////

#if !MICROPY_VFS



mp_obj_t mp_vfs_listdir(size_t n_args, const mp_obj_t *args) {
    mp_obj_t dir_list = mp_obj_new_list(0, NULL);
    char* tmp = (char*)malloc(200);
    if(!tmp)
        mp_raise_OSError(MP_ENOMEM);
    memset(tmp,0,200);
    uint32_t ret =API_FS_GetCurDir(200,tmp);
    if(ret != 0)
    {
        free(tmp);
        mp_raise_OSError(MP_EIO);
    }
    char* dirToList = "/";
    if(n_args != 0 )
        dirToList = (char*)mp_obj_str_get_str(args[0]);
    else
    {
        dirToList = tmp;
    }
    Dir_t* dir = API_FS_OpenDir((const char*)dirToList);
    Dirent_t* dirent = NULL;
    while( (dirent = API_FS_ReadDir(dir)) )
    {
        // Trace(1,"dir name:%s",dirent->d_name);
        mp_obj_t dirStr = mp_obj_new_str(dirent->d_name,strlen(dirent->d_name));
        mp_obj_list_append(dir_list, dirStr);
    }
    API_FS_CloseDir(dir);
    API_FS_OpenDir(tmp);
    free(tmp);
    return dir_list;
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_vfs_listdir_obj, 0, 1, mp_vfs_listdir);


mp_obj_t mp_vfs_mkdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if(strcmp(path,"/")==0 || strcmp(path,"/t")==0)
    {
        mp_raise_OSError(MP_EEXIST);
    }
    int32_t ret = API_FS_Mkdir(path,0);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_mkdir_obj, mp_vfs_mkdir);

mp_obj_t mp_vfs_remove(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if(strcmp(path,"/")==0 || strcmp(path,"/t")==0)
    {
        mp_raise_OSError(MP_EINVAL);
    }
    int32_t ret = API_FS_Delete(path);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_remove_obj, mp_vfs_remove);

mp_obj_t mp_vfs_rename(mp_obj_t old_path_in, mp_obj_t new_path_in) {
    const char* path_old = mp_obj_str_get_str(old_path_in);
    const char* path_new = mp_obj_str_get_str(new_path_in);
    int32_t ret = API_FS_Rename(path_old,path_new);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_vfs_rename_obj, mp_vfs_rename);

mp_obj_t mp_vfs_rmdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    if(strcmp(path,"/")==0 || strcmp(path,"/t")==0)
    {
        mp_raise_OSError(MP_EINVAL);
    }
    int32_t ret = API_FS_Rmdir(path);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_rmdir_obj, mp_vfs_rmdir);


mp_obj_t mp_vfs_chdir(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    int32_t ret = API_FS_ChangeDir(path);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_chdir_obj, mp_vfs_chdir);

mp_obj_t mp_vfs_getcwd(void) {
    char* tmp = (char*)malloc(200);
    if(!tmp)
        mp_raise_OSError(MP_ENOMEM);
    memset(tmp,0,200);
    uint32_t ret =API_FS_GetCurDir(200,tmp);
    if(ret != 0)
    {
        free(tmp);
        mp_raise_OSError(MP_EIO);
    }
    mp_obj_t retVal = mp_obj_new_str(tmp,strlen(tmp));
    free(tmp);
    return retVal;
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_vfs_getcwd_obj, mp_vfs_getcwd);

mp_obj_t mp_vfs_stat(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    API_FS_INFO info;
    int32_t ret = API_FS_GetFSInfo(path,&info);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = mp_obj_new_int(info.totalSize);
    t->items[1] = mp_obj_new_int(info.usedSize);
    return MP_OBJ_FROM_PTR(t);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_stat_obj, mp_vfs_stat);

mp_obj_t mp_vfs_statvfs(mp_obj_t path_in) {
    const char* path = mp_obj_str_get_str(path_in);
    API_FS_INFO info;
    int32_t ret = API_FS_GetFSInfo(path,&info);
    if(ret != 0)
        mp_raise_OSError(MP_EIO);
    mp_obj_tuple_t *t = MP_OBJ_TO_PTR(mp_obj_new_tuple(10, NULL));
    t->items[0] = mp_obj_new_int(info.totalSize);
    t->items[1] = mp_obj_new_int(info.usedSize);
    return MP_OBJ_FROM_PTR(t);
}
MP_DEFINE_CONST_FUN_OBJ_1(mp_vfs_statvfs_obj, mp_vfs_statvfs);

#endif //#if !MICROPY_VFS
///////////////////////////////////////////////////////////////////

#if MICROPY_PY_OS_DUPTERM
STATIC mp_obj_t os_dupterm_notify(mp_obj_t obj_in) {
    (void)obj_in;
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
    #else
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&mp_vfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&mp_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&mp_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_chdir), MP_ROM_PTR(&mp_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd), MP_ROM_PTR(&mp_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&mp_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&mp_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&mp_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_statvfs), MP_ROM_PTR(&mp_vfs_statvfs_obj) },
    #endif
};

STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);

const mp_obj_module_t uos_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
