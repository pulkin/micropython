#include "api_event.h"

extern int sim_status;

void notify_no_sim(API_Event_t* event);
void notify_registered_home(API_Event_t* event);
void notify_registered_roaming(API_Event_t* event);
void notify_sms_list(API_Event_t* event);

#define MAX_SMS_LIST_ATTEMPTS 10
#define REQUIRES_NETWORK_REGISTRATION do {if (!sim_status) {mp_raise_ValueError("Network is not available: is SIM card inserted?"); return mp_const_none;}} while(0)
#define REQUIRES_VALID_SMS_STATUS(bits) do {if (bitsum(bits) != 1) {return mp_const_none;}} while(0)
