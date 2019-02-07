#include "api_event.h"

extern int sim_status;

void notify_no_sim(API_Event_t* event);
void notify_registered_home(API_Event_t* event);
void notify_registered_roaming(API_Event_t* event);

#define REQUIRES_NETWORK_REGISTRATION if (!sim_status) {mp_raise_ValueError("Network is not available: is SIM card inserted?"); return mp_const_none;}

