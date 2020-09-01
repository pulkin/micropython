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
#include "timeout.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objexcept.h"
#include "lib/timeutils/timeutils.h"

#include "api_gps.h"
#include "api_os.h"
#include "api_event.h"
#include "gps_parse.h"
#include "gps.h"
#include "minmea.h"
#include "time.h"

#include "py/mperrno.h"

STATIC mp_obj_t modgps_off(void);
mp_obj_t gps_callback = mp_const_none;

void modgps_init0(void) {
    modgps_off();
    gps_callback = mp_const_none;
}

// ------
// Notify
// ------

GPS_Info_t* gpsInfo = NULL;

void modgps_notify_gps_update(API_Event_t* event) {
    GPS_Update(event->pParam1,event->param1);

    if (gps_callback && gps_callback != mp_const_none) {
        mp_sched_schedule(gps_callback, MP_OBJ_NEW_SMALL_INT(0));
    }
}

STATIC mp_obj_t modgps_on_update(mp_obj_t callable) {
    // ========================================
    // Sets a callback on GPS update.
    // Args:
    //     callback (Callable): a callback to
    //     execute on GPS data received from UART.
    // ========================================
    gps_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modgps_on_update_obj, modgps_on_update);

STATIC mp_obj_t modgps_on_update(mp_obj_t callable) {
    // ========================================
    // Sets a callback on GPS update.
    // Args:
    //     callback (Callable): a callback to
    //     execute on GPS data received from UART.
    // ========================================
    gps_callback = callable;
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(modgps_on_update_obj, modgps_on_update);

// -------
// Methods
// -------

#define USE_GPS 1
#define USE_GLONASS 2
#define USE_BEIDOU 4
#define USE_GALILEO 8

STATIC mp_obj_t modgps_on(size_t n_args, const mp_obj_t *arg) {
    // ========================================
    // Turns GPS on.
    // Args:
    //     timeout (int): timeout in seconds;
    //     mode(int): navigation systems to use, GPS, GLONASS, BEIDOU, GALILEO as bitmask
    // Raises:
    //     ValueError if failed to turn GPS on.
    // ========================================
    uint32_t timeout = 0;
    uint8_t search_mode = USE_GPS;

    if (n_args == 0) {
        timeout = DEFAULT_GPS_TIMEOUT;
    }
    else  {
        timeout = mp_obj_get_int(arg[0]);

        if (n_args == 2) {
            search_mode = mp_obj_get_int(arg[1]);
        }

    }

    timeout *= 1000 * CLOCKS_PER_MSEC;
    gpsInfo = Gps_GetInfo();
    gpsInfo->rmc.latitude.value = 0;
    gpsInfo->rmc.longitude.value = 0;
    GPS_Init();
    GPS_Open(NULL);

    GPS_SetSearchMode(!!(search_mode & USE_GPS), !!(search_mode & USE_GLONASS),
                     !!(search_mode & USE_BEIDOU), !!(search_mode & USE_GALILEO));

    WAIT_UNTIL(gpsInfo->rmc.latitude.value, timeout, 100, mp_raise_OSError(MP_ETIMEDOUT));

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modgps_on_obj, 0, 2, modgps_on);

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
        mp_raise_ValueError("No firmware info");
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
        mp_obj_new_float(latitude),
        mp_obj_new_float(longitude),
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

STATIC mp_obj_t modgps_time(void) {
    // ========================================
    // GPS time.
    // Returns:
    //     Seconds since 2000 as int.
    // ========================================
    REQUIRES_GPS_ON;

    struct minmea_date date = gpsInfo->rmc.date;
    struct minmea_time time = gpsInfo->rmc.time;

    mp_uint_t result = timeutils_mktime(date.year + 2000, date.month, date.day, time.hours, time.minutes, time.seconds);
    // Note: the module may report dates between 1980 and 2000 as well: they will be mapped onto the time frame 2080-2100
    return mp_obj_new_int_from_uint(result);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_time_obj, modgps_time);

STATIC mp_obj_t _get_time(struct minmea_date date, struct minmea_time time) {
    mp_uint_t result = timeutils_mktime(date.year + 2000, date.month, date.day, time.hours, time.minutes, time.seconds);
    // Note: the module may report dates between 1980 and 2000 as well: they will be mapped onto the time frame 2080-2100
    return mp_obj_new_int_from_uint(result);
}

STATIC mp_obj_t _get_float(struct minmea_float f) {
    return mp_obj_new_float(minmea_tofloat(&f));
}

STATIC mp_obj_t _get_prn(int* prn) {
    uint8_t prn_u[12];
    for (int i = 0; i < 12; i++) prn_u[i] = prn[i];
    return mp_obj_new_bytearray(sizeof(prn), prn);
}

STATIC mp_obj_t _get_sat_info(struct minmea_sat_info s) {
    mp_obj_t tuple[4] = {
        mp_obj_new_int(s.nr),
        mp_obj_new_int(s.elevation),
        mp_obj_new_int(s.azimuth),
        mp_obj_new_int(s.snr),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_rmc(struct minmea_sentence_rmc s) {
    mp_obj_t tuple[7] = {
        _get_time(s.date, s.time),
        mp_obj_new_bool(s.valid),
        _get_float(s.latitude),
        _get_float(s.longitude),
        _get_float(s.speed),
        _get_float(s.course),
        _get_float(s.variation),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gsa(struct minmea_sentence_gsa s) {
    mp_obj_t tuple[6] = {
        mp_obj_new_int_from_uint(s.mode),
        mp_obj_new_int(s.fix_type),
        _get_prn(s.sats),
        _get_float(s.pdop),
        _get_float(s.hdop),
        _get_float(s.vdop),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gga(struct minmea_sentence_gga s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[11] = {
        _get_time(no_date, s.time),
        _get_float(s.latitude),
        _get_float(s.longitude),
        mp_obj_new_int(s.fix_quality),
        mp_obj_new_int(s.satellites_tracked),
        _get_float(s.hdop),
        _get_float(s.altitude),
        mp_obj_new_int_from_uint(s.altitude_units),
        _get_float(s.height),
        mp_obj_new_int_from_uint(s.height_units),
        _get_float(s.dgps_age),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gll(struct minmea_sentence_gll s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[5] = {
        _get_float(s.latitude),
        _get_float(s.longitude),
        _get_time(no_date, s.time),
        mp_obj_new_int_from_uint(s.status),
        mp_obj_new_int_from_uint(s.mode),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gst(struct minmea_sentence_gst s) {
    struct minmea_date no_date = {1, 1, 0};

    mp_obj_t tuple[8] = {
        _get_time(no_date, s.time),
        _get_float(s.rms_deviation),
        _get_float(s.semi_major_deviation),
        _get_float(s.semi_minor_deviation),
        _get_float(s.semi_major_orientation),
        _get_float(s.latitude_error_deviation),
        _get_float(s.longitude_error_deviation),
        _get_float(s.altitude_error_deviation),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_gsv(struct minmea_sentence_gsv s) {
    mp_obj_t sat_info_tuple[4];
    for (int i = 0; i < 4; i++)
        sat_info_tuple[i] = _get_sat_info(s.sats[i]);

    mp_obj_t tuple[4] = {
        mp_obj_new_int(s.total_msgs),
        mp_obj_new_int(s.msg_nr),
        mp_obj_new_int(s.total_sats),
        mp_obj_new_tuple(4, sat_info_tuple),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_vtg(struct minmea_sentence_vtg s) {
    mp_obj_t tuple[5] = {
        _get_float(s.true_track_degrees),
        _get_float(s.magnetic_track_degrees),
        _get_float(s.speed_knots),
        _get_float(s.speed_kph),
        mp_obj_new_int_from_uint((uint8_t) s.faa_mode),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t _get_zda(struct minmea_sentence_zda s) {
    mp_obj_t tuple[3] = {
        _get_time(s.date, s.time),
        mp_obj_new_int(s.hour_offset),
        mp_obj_new_int(s.minute_offset),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC mp_obj_t modgps_nmea_data(void) {
    // ========================================
    // NMEA data in a tuple.
    // Returns:
    //     A tuple with rmc, gsa, gga, gll,
    //     gst, gsv, vtg and zda messages.
    // ========================================
    REQUIRES_GPS_ON;

    mp_obj_t gsa_tuple[GPS_PARSE_MAX_GSA_NUMBER];
    for (int i = 0; i < GPS_PARSE_MAX_GSA_NUMBER; i++)
        gsa_tuple[i] = _get_gsa(gpsInfo->gsa[i]);

    mp_obj_t gsv_tuple[GPS_PARSE_MAX_GSV_NUMBER];
    for (int i = 0; i < GPS_PARSE_MAX_GSV_NUMBER; i++)
        gsv_tuple[i] = _get_gsv(gpsInfo->gsv[i]);

    mp_obj_t tuple[8] = {
        _get_rmc(gpsInfo->rmc),
        mp_obj_new_tuple(GPS_PARSE_MAX_GSA_NUMBER, gsa_tuple),
        _get_gga(gpsInfo->gga),
        _get_gll(gpsInfo->gll),
        _get_gst(gpsInfo->gst),
        mp_obj_new_tuple(GPS_PARSE_MAX_GSV_NUMBER, gsv_tuple),
        _get_vtg(gpsInfo->vtg),
        _get_zda(gpsInfo->zda),
    };
    return mp_obj_new_tuple(sizeof(tuple) / sizeof(mp_obj_t), tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modgps_nmea_data_obj, modgps_nmea_data);

STATIC const mp_map_elem_t mp_module_gps_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gps) },

    // GNSS
    { MP_ROM_QSTR(MP_QSTR_GLONASS),          MP_ROM_INT(USE_GLONASS) },
    { MP_ROM_QSTR(MP_QSTR_GPS),              MP_ROM_INT(USE_GPS) },
    { MP_ROM_QSTR(MP_QSTR_GALILEO),          MP_ROM_INT(USE_GALILEO) },
    { MP_ROM_QSTR(MP_QSTR_BEIDOU),           MP_ROM_INT(USE_BEIDOU) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_on), (mp_obj_t)&modgps_on_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_off), (mp_obj_t)&modgps_off_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_firmware_version), (mp_obj_t)&modgps_get_firmware_version_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_location), (mp_obj_t)&modgps_get_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_last_location), (mp_obj_t)&modgps_get_last_location_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_get_satellites), (mp_obj_t)&modgps_get_satellites_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_time), (mp_obj_t)&modgps_time_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_nmea_data), (mp_obj_t)&modgps_nmea_data_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_on_update), (mp_obj_t)&modgps_on_update},
};

STATIC MP_DEFINE_CONST_DICT(mp_module_gps_globals, mp_module_gps_globals_table);

const mp_obj_module_t gps_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_gps_globals,
};
