
#ifndef MICROPY_INCLUDED_GPRS_A9_MODMACHINE_H
#define MICROPY_INCLUDED_GPRS_A9_MODMACHINE_H

#include "py/obj.h"
#include "api_event.h"

extern const mp_obj_type_t machine_pin_type;
extern Power_On_Cause_t powerOnCause;

void notify_power_on(API_Event_t* event);

#endif

