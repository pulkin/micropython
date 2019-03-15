#include "api_event.h"
#include "gps_parse.h"
#include "py/obj.h"

extern const mp_obj_type_t mp_type_CellularError;

void modgps_notify_gps_update(API_Event_t* event);

#define MAX_GPS_TIMEOUT 5000
#define REQUIRES_VALID_GPS_INFO do { if (!gpsInfo) {mp_raise_GPSError("No GPS data available: did you run gps.on()?"); return mp_const_none;}} while(0)
#define REQUIRES_GPS_ON do { if (!GPS_IsOpen()) {mp_raise_GPSError("GPS is off"); return mp_const_none;}} while(0)
