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

#include "modgps.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objexcept.h"

#include "api_gps.h"
#include "api_os.h"
#include "api_event.h"
#include "gps_parse.h"
#include "gps.h"
#include "time.h"

void modgps_init0(void) {
    GPS_Close();
}

// ------
// Notify
// ------

GPS_Info_t* gpsInfo = NULL;

void modgps_notify_gps_update(API_Event_t* event) {
    GPS_Update(event->pParam1,event->param1);
}

// ----------
// Exceptions
// ----------

MP_DEFINE_EXCEPTION(GPSError, OSError)

NORETURN void mp_raise_GPSError(const char *msg) {
    mp_raise_msg(&mp_type_GPSError, msg);
}

// -------
// Methods
// -------

STATIC mp_obj_t modgps_on(size_t n_args, const mp_obj_t *arg) {
    // ========================================
    // Turns GPS on.
    // Args:
    //     timeout (int): timeout in seconds;
    // Raises:
    //     ValueError if failed to turn GPS on.
    // ========================================
    uint32_t timeout = 0;
    if (n_args == 0) {
        timeout = DEFAULT_GPS_TIMEOUT;
    } else {
        timeout = mp_obj_get_int(arg[0]);
    }
    timeout *= 1000 * CLOCKS_PER_MSEC;
    gpsInfo = Gps_GetInfo();
    gpsInfo->rmc.latitude.value = 0;
    gpsInfo->rmc.longitude.value = 0;
    GPS_Init();
    GPS_Open(NULL);
    clock_t time = clock();
    while (clock() - time < timeout) {
        if (gpsInfo->rmc.latitude.value != 0) {
            break;
        }
        OS_Sleep(100);
    }
    if (gpsInfo->rmc.latitude.value == 0) {
        mp_raise_GPSError("Failed to start GPS");
    }
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modgps_on_obj, 0, 1, modgps_on);

STATIC mp_obj_t modgps_off(void) {
    // ========================================
    // Turns GPS off.
    // ========================================
    GPS_Close();
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_off_obj, modgps_off);

STATIC mp_obj_t modgps_get_firmware_version(void) {
    // ========================================
    // Retrieves firmware version.
    // Returns:
    //     The firmware version as a string.
    // Raises:
    //     ValueError if GPS was off.
    // ========================================
    char buffer[300];
    if (!GPS_GetVersion(buffer, 150)) {
        mp_raise_GPSError("Failed to get the firmware version: did you run gps.on()?");
        return mp_const_none;
    }
    return mp_obj_new_str(buffer, strlen(buffer));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_firmware_version_obj, modgps_get_firmware_version);

STATIC mp_obj_t modgps_get_last_location(void) {
    // ========================================
    // Retrieves the last GPS location.
    // Returns:
    //     Location reported by GPS: longitude and latitude (degrees).
    // ========================================
    REQUIRES_VALID_GPS_INFO;

    int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
    double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
    temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
    double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;
    mp_obj_t tuple[2] = {
        mp_obj_new_float(longitude),
        mp_obj_new_float(latitude),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_last_location_obj, modgps_get_last_location);

STATIC mp_obj_t modgps_get_location(void) {
    // ========================================
    // Retrieves the current GPS location.
    // Returns:
    //     Location reported by GPS: longitude and latitude (degrees).
    // ========================================
    REQUIRES_GPS_ON;

    return modgps_get_last_location();
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_location_obj, modgps_get_location);

STATIC mp_obj_t modgps_get_satellites(void) {
    // ========================================
    // Retrieves the number of visible GPS satellites.
    // Returns:
    //     The number of visible GPS satellites.
    // ========================================
    REQUIRES_GPS_ON;

    mp_obj_t tuple[2] = {
        mp_obj_new_int(gpsInfo->gga.satellites_tracked),
        mp_obj_new_int(gpsInfo->gsv[0].total_sats),
    };
    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_get_satellites_obj, modgps_get_satellites);

STATIC const mp_map_elem_t mp_module_gps_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__i), MP_OBJ_NEW_QSTR(MP_QSTR_gps) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_GPSError), (mp_obj_t)MP_ROM_PTR(&mp_type_GPSError) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&modgps_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&modgps_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_firmware_version), (mp_obj_t)&modgps_get_firmware_version_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_location), (mp_obj_t)&modgps_get_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_last_location), (mp_obj_t)&modgps_get_last_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_satellites), (mp_obj_t)&modgps_get_satellites_obj },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gps_globals, mp_module_gps_globals_table);

const mp_obj_module_t gps_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gps_globals,
};
