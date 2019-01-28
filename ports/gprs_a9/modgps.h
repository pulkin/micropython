#define MAX_GPS_POLL_ATTEMPTS 10

#define REQUIRES_VALID_GPS_INFO(info) if (!(info)) {mp_raise_ValueError("No GPS data available: did you run gps.on()?"); return mp_const_none;}

