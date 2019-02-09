#include "api_event.h"
#include "gps_parse.h"

extern GPS_Info_t* gpsInfo;

void notify_gps_update(API_Event_t* event);

#define MAX_GPS_POLL_ATTEMPTS 10
#define REQUIRES_VALID_GPS_INFO do { if (!gpsInfo) {mp_raise_ValueError("No GPS data available: did you run gps.on()?"); return mp_const_none;}} while(0)
#define REQUIRES_GPS_ON do { if (!GPS_IsOpen()) {mp_raise_ValueError("GPS is off"); return mp_const_none;}} while(0)
