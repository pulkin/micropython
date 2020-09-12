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

#include "api_event.h"
#include "py/obj.h"

extern const mp_obj_type_t mp_type_CellularError;

void modcellular_init0(void);

void modcellular_notify_no_sim(API_Event_t* event);
void modcellular_notify_sim_drop(API_Event_t* event);

void modcellular_notify_reg_home(API_Event_t* event);
void modcellular_notify_reg_roaming(API_Event_t* event);
void modcellular_notify_reg_searching(API_Event_t* event);
void modcellular_notify_reg_denied(API_Event_t* event);
void modcellular_notify_dereg(API_Event_t* event);

void modcellular_notify_det(API_Event_t* event);
void modcellular_notify_att_failed(API_Event_t* event);
void modcellular_notify_att(API_Event_t* event);

void modcellular_notify_deact(API_Event_t* event);
void modcellular_notify_act_failed(API_Event_t* event);
void modcellular_notify_act(API_Event_t* event);

void modcellular_notify_ntwlist(API_Event_t* event);

void modcellular_notify_sms_list(API_Event_t* event);
void modcellular_notify_sms_sent(API_Event_t* event);
void modcellular_notify_sms_error(API_Event_t* event);
void modcellular_notify_sms_receipt(API_Event_t* event);

void modcellular_notify_signal(API_Event_t* event);
void modcellular_notify_cell_info(API_Event_t* event);

void modcellular_notify_call_incoming(API_Event_t* event);
void modcellular_notify_call_hangup(API_Event_t* event);

void modcellular_notify_ussd_sent(API_Event_t* event);
void modcellular_notify_ussd_failed(API_Event_t* event);
void modcellular_notify_incoming_ussd(API_Event_t* event);

#define TIMEOUT_SMS_LIST 10000
#define TIMEOUT_SMS_SEND 10000
#define TIMEOUT_GPRS_ATTACHMENT 15000
#define TIMEOUT_GPRS_ACTIVATION 10000
#define TIMEOUT_FLIGHT_MODE 10000
#define TIMEOUT_LIST_OPERATORS 15000
#define TIMEOUT_REG 10000
#define TIMEOUT_STATIONS 2000
#define TIMEOUT_USSD_RESPONSE 5000

#define REQUIRES_NETWORK_REGISTRATION do {if (!network_status) {mp_raise_RuntimeError("Network is not available: is SIM card inserted?"); return mp_const_none;}} while(0)

