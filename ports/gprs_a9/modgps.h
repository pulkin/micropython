#include "api_event.h"
#include "gps_parse.h"

extern GPS_Info_t* gpsInfo;

void notify_gps_update(API_Event_t* event);

#define MAX_GPS_POLL_ATTEMPTS 10
#define REQUIRES_VALID_GPS_INFO if (!gpsInfo) {mp_raise_ValueError("No GPS data available: did you run gps.on()?"); return mp_const_none;}

