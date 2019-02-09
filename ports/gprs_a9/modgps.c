#include "modgps.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"

#include "api_gps.h"
#include "api_os.h"
#include "api_event.h"
#include "gps_parse.h"
#include "gps.h"

// ------
// Notify
// ------

GPS_Info_t* gpsInfo = NULL;

void notify_gps_update(API_Event_t* event) {
    GPS_Update(event->pParam1,event->param1);
}

// -------
// Methods
// -------

STATIC mp_obj_t on(void) {
    // ========================================
    // Turns GPS on.
    // Raises:
    //     ValueError if failed to turn GPS on.
    // ========================================
    GPS_Init();
    GPS_Open(NULL);
    gpsInfo = Gps_GetInfo();
    uint8_t attempts = 0;
    while (gpsInfo->rmc.latitude.value == 0) {
        attempts ++;
        if (attempts == MAX_GPS_POLL_ATTEMPTS) {
            mp_raise_ValueError("Failed to start GPS");
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

STATIC mp_obj_t get_last_location(void) {
    // ========================================
    // Retrieves the last GPS location.
    // Returns:
    //     Location reported by GPS: latitude and longitude (degrees).
    // ========================================
    REQUIRES_VALID_GPS_INFO;

    int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
    double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
    temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
    double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;
    mp_obj_t tuple[2] = {
        mp_obj_new_float(latitude),
        mp_obj_new_float(longitude),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_last_location_obj, get_last_location);

STATIC mp_obj_t get_location(void) {
    // ========================================
    // Retrieves the current GPS location.
    // Returns:
    //     Location reported by GPS: latitude and longitude (degrees).
    // ========================================
    REQUIRES_GPS_ON;

    return get_last_location();
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_location_obj, get_location);

STATIC mp_obj_t get_satellites(void) {
    // ========================================
    // Retrieves the number of visible GPS satellites.
    // Returns:
    //     The number of visible GPS satellites.
    // ========================================
    REQUIRES_VALID_GPS_INFO;

    mp_obj_t tuple[2] = {
        mp_obj_new_int(gpsInfo->gga.satellites_tracked),
        mp_obj_new_int(gpsInfo->gsv[0].total_sats),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_satellites_obj, get_satellites);

STATIC const mp_map_elem_t mp_module_gps_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gps) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_firmware_version), (mp_obj_t)&get_firmware_version_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_location), (mp_obj_t)&get_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_last_location), (mp_obj_t)&get_last_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_satellites), (mp_obj_t)&get_satellites_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gps_globals, mp_module_gps_globals_table);

const mp_obj_module_t gps_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gps_globals,
};
