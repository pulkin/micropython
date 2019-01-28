#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_gps.h"
#include "api_os.h"
#include "gps_parse.h"

#include "modgps.h"
#include "gps.h"


STATIC mp_obj_t on(void) {
    // ========================================
    // Turns GPS on.
    // Raises:
    //     ValueError if failed to turn GPS on.
    // ========================================
    GPS_Init();
    GPS_Open(NULL);
    GPS_Info_t* gpsInfo = Gps_GetInfo();
    uint8_t attempts = 0;
    while (gpsInfo->rmc.latitude.value == 0) {
        attempts ++;
        if (attempts == MAX_GPS_POLL_ATTEMPTS) {
            mp_raise_ValueError("Failed to start GPS");
            return mp_const_none;
        }
        OS_Sleep(1000);
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(on_obj, on);

STATIC mp_obj_t off(void) {
    // ========================================
    // Turns GPS off.
    // ========================================
    GPS_Close();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(off_obj, off);

STATIC mp_obj_t get_firmware_version(void) {
    // ========================================
    // Retrieves firmware version.
    // Returns:
    //     The firmware version as a string.
    // Raises:
    //     ValueError if GPS was off.
    // ========================================
    char buffer[300];
    if (!GPS_GetVersion(buffer,150)) {
        mp_raise_ValueError("Failed to get the firmware version: did you run gps.on()?");
        return mp_const_none;
    }
    return mp_obj_new_str(buffer, strlen(buffer));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_firmware_version_obj, get_firmware_version);

STATIC const mp_map_elem_t mp_module_gps_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gps) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_firmware_version), (mp_obj_t)&get_firmware_version_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gps_globals, mp_module_gps_globals_table);

const mp_obj_module_t gps_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gps_globals,
};
