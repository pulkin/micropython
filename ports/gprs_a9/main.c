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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "lib/utils/pyexec.h"

#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"
#include "buffer.h"
#include "api_network.h"
#include "time.h"
#include "api_fs.h"
#include "fatal.h"

#include "moduos.h"
#include "mphalport.h"
#include "mpconfigport.h"
#include "modcellular.h"
#include "modgps.h"
#include "modmachine.h"

#define AppMain_TASK_STACK_SIZE    (2048 * 2)
#define AppMain_TASK_PRIORITY      0
#define MICROPYTHON_TASK_STACK_SIZE     (2048 * 4)
#define MICROPYTHON_TASK_PRIORITY       1
#define UART_CIRCLE_FIFO_BUFFER_MAX_LENGTH 2048

#define MICROPYTHON_HEAP_MAX_SIZE (1024 * 2048)
#define MICROPYTHON_HEAP_MIN_SIZE (2048)

STATIC void* heap;

HANDLE mainTaskHandle  = NULL;
HANDLE microPyTaskHandle = NULL;
Buffer_t fifoBuffer; // ringbuf
uint8_t  fifoBufferData[UART_CIRCLE_FIFO_BUFFER_MAX_LENGTH];

typedef enum
{
    MICROPY_EVENT_ID_UART_RECEIVED = 1, //param1: length, param2:data(uint8_t*)
    MICROPY_EVENT_ID_UART_MAX
} MicroPy_Event_ID_t;

typedef struct
{
    MicroPy_Event_ID_t id;
    uint32_t param1;
    uint8_t* pParam1;
} MicroPy_Event_t;

extern mp_uint_t gc_helper_get_regs_and_sp(mp_uint_t*);

#if MICROPY_ENABLE_COMPILER
/*void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    mp_printf(&mp_plat_print, "do_str %s\n", src);
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}*/
#endif

STATIC void *stack_top;

void gc_collect(void) {
    //ESP8266-style
    gc_collect_start();
    mp_uint_t regs[8];
    mp_uint_t sp = gc_helper_get_regs_and_sp(regs);
    gc_collect_root((void**)sp, (mp_uint_t)(stack_top - sp) / sizeof(mp_uint_t));
    gc_collect_end();
}

void NORETURN nlr_jump_fail(void *val) {
    mp_fatal_error(MP_FATAL_REASON_NLR_JUMP_FAIL, val);
}

#if MICROPY_ENABLE_GC
void* mp_allocate_heap(uint32_t* size) {
    uint32_t h_size = MICROPYTHON_HEAP_MAX_SIZE;
    void* ptr = NULL;
    while (!ptr) {
        if (h_size < MICROPYTHON_HEAP_MIN_SIZE) {
            mp_fatal_error(MP_FATAL_REASON_HEAP_INIT, NULL);
        }
        ptr = OS_Malloc(h_size);
        if (!ptr) {
            h_size = h_size >> 1;
        }
    }
    size[0] = h_size;
    return ptr;
}
#endif

/*
void NORETURN __fatal_error(const char *msg) {
    Assert(false,msg);
    while (1);//never reach here actully
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    Trace(1,"Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif
*/

void MicroPyTask(void *pData) {

    OS_Task_Info_t info;
    OS_GetTaskInfo(microPyTaskHandle, &info);
    MicroPy_Event_t* event;
    Buffer_Init(&fifoBuffer, fifoBufferData, sizeof(fifoBufferData));

soft_reset:
    mp_stack_ctrl_init();
    stack_top = (void*) info.stackTop + info.stackSize * 4;
    mp_stack_set_top((void *) stack_top);
    mp_stack_set_limit(MICROPYTHON_TASK_STACK_SIZE * 4 - 1024);
#if MICROPY_ENABLE_GC
    uint32_t heap_size;
    heap = mp_allocate_heap(&heap_size);
    gc_init(heap, heap + heap_size);
#endif
    mp_init();
    moduos_init0();
    modcellular_init0();
    modgps_init0();
    modmachine_init0();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_));
    mp_obj_list_init(mp_sys_argv, 0);
    // Startup scripts
    pyexec_frozen_module("_boot.py");
    pyexec_file_if_exists("boot.py");
    if (pyexec_mode_kind == PYEXEC_MODE_FRIENDLY_REPL) {
        pyexec_file_if_exists("main.py");
    }
    // pyexec_event_repl_init();
    
    // while (1) if (OS_WaitEvent(microPyTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER)) {
    while (1) {

        /* switch(event->id) {
            case MICROPY_EVENT_ID_UART_RECEIVED: {
                uint8_t c;
                Trace(1, "micropy task received data length: %d", event->param1);
                while (Buffer_Gets(&fifoBuffer, &c, 1))
                    if (pyexec_event_repl_process_char((int)c)) {
                        reset = 1;
                        break;
                    }
                Trace(1, "REPL complete");
                break;
            }
            default:
                break;
        }

        OS_Free(event->pParam1);
        OS_Free(event);*/

        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

#if MICROPY_ENABLE_GC
    gc_sweep_all();
#endif
    mp_deinit();
    OS_Free(heap);
    mp_hal_stdout_tx_str("PYB: soft reboot\r\n");
    mp_hal_delay_us(10000); // allow UART to flush output

    goto soft_reset;
}

void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_POWER_ON:
            modmachine_notify_power_on(pEvent);
            break;

        case API_EVENT_ID_KEY_DOWN:
            modmachine_notify_power_key_down(pEvent);
            break;
        
        case API_EVENT_ID_KEY_UP:
            modmachine_notify_power_key_up(pEvent);
            break;

        // Network
        // =======

        case API_EVENT_ID_NO_SIMCARD:
            modcellular_notify_no_sim(pEvent);
            break;

        case API_EVENT_ID_SIMCARD_DROP:
            modcellular_notify_sim_drop(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
            modcellular_notify_reg_home(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            modcellular_notify_reg_roaming(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
            modcellular_notify_reg_searching(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_DENIED:
            modcellular_notify_reg_denied(pEvent);
            break;

        case API_EVENT_ID_NETWORK_REGISTER_NO:
            // TODO: WTF is this?
            modcellular_notify_reg_denied(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DEREGISTER:
            modcellular_notify_dereg(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DETACHED:
            modcellular_notify_det(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ATTACH_FAILED:
            modcellular_notify_att_failed(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ATTACHED:
            modcellular_notify_att(pEvent);
            break;

        case API_EVENT_ID_NETWORK_DEACTIVED:
            modcellular_notify_deact(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATE_FAILED:
            modcellular_notify_act_failed(pEvent);
            break;

        case API_EVENT_ID_NETWORK_ACTIVATED:
            modcellular_notify_act(pEvent);
            break;

        case API_EVENT_ID_NETWORK_AVAILABEL_OPERATOR:
            modcellular_notify_ntwlist(pEvent);
            break;

        // SMS
        // ===

        case API_EVENT_ID_SMS_SENT:
            modcellular_notify_sms_sent(pEvent);
            break;

        case API_EVENT_ID_SMS_ERROR:
            modcellular_notify_sms_error(pEvent);
            break;

        case API_EVENT_ID_SMS_LIST_MESSAGE:
            modcellular_notify_sms_list(pEvent);
            break;

        case API_EVENT_ID_SMS_RECEIVED:
            modcellular_notify_sms_receipt(pEvent);
            break;

        // Signal
        // ======

        case API_EVENT_ID_SIGNAL_QUALITY:
            modcellular_notify_signal(pEvent);
            break;

        case API_EVENT_ID_NETWORK_CELL_INFO:
            modcellular_notify_cell_info(pEvent);
            break;

        // Call
        // ====

        case API_EVENT_ID_CALL_INCOMING:
            modcellular_notify_call_incoming(pEvent);
            break;

        case API_EVENT_ID_CALL_HANGUP:
            modcellular_notify_call_hangup(pEvent);
            break;

        // USSD
        // ====
        case API_EVENT_ID_USSD_SEND_SUCCESS:
            modcellular_notify_ussd_sent(pEvent);
            break;

        case API_EVENT_ID_USSD_SEND_FAIL:
            modcellular_notify_ussd_failed(pEvent);
            break;

        case API_EVENT_ID_USSD_IND:
            modcellular_notify_incoming_ussd(pEvent);
            break;

        // UART
        // ====
        case API_EVENT_ID_UART_RECEIVED:
            /*Trace(1,"UART%d received:%d,%s",pEvent->param1,pEvent->param2,pEvent->pParam1);
            if(pEvent->param1 == UART1)
            {
                MicroPy_Event_t* event = (MicroPy_Event_t*)OS_Malloc(sizeof(MicroPy_Event_t));
                if(!event)
                {
                    Trace(1,"malloc fail");
                    break;
                }
                for(uint16_t i=0; i<pEvent->param2; ++i)
                {
                   if (pEvent->pParam1[i] == mp_interrupt_char) {
                        // inline version of mp_keyboard_interrupt();
                        MP_STATE_VM(mp_pending_exception) = MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
                        #if MICROPY_ENABLE_SCHEDULER
                        if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
                            MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
                        }
                        #endif
                   }
                   else
                   {
                       Buffer_Puts(&fifoBuffer,pEvent->pParam1+i,1);
                       len++;
                   }
                }
                memset((void*)event,0,sizeof(MicroPy_Event_t));
                event->id = MICROPY_EVENT_ID_UART_RECEIVED;
                event->param1 = len;
                OS_SendEvent(microPyTaskHandle, (void*) event, OS_TIME_OUT_WAIT_FOREVER, 0);
            }*/
            break;

        // GPS
        // ===
        case API_EVENT_ID_GPS_UART_RECEIVED:
            modgps_notify_gps_update(pEvent);
            break;

        default:
            break;
    }
}


void AppMainTask(void *pData)
{
    API_Event_t* event=NULL;
    
    TIME_SetIsAutoUpdateRtcTime(true);

    microPyTaskHandle = OS_CreateTask(MicroPyTask, NULL, NULL, MICROPYTHON_TASK_STACK_SIZE, MICROPYTHON_TASK_PRIORITY, 0, 0, "mpy Task");                                    
    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
    }
}
void _Main(void)
{
    mainTaskHandle = OS_CreateTask(AppMainTask, NULL, NULL, AppMain_TASK_STACK_SIZE, AppMain_TASK_PRIORITY, 0, 0, "main Task");
    OS_SetUserMainHandle(&mainTaskHandle);
}



#if MICROPY_DEBUG_VERBOSE

char trace_debug_buffer[64];
size_t trace_debug_buffer_len = 0;

int DEBUG_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int ret = mp_vprintf(MICROPY_DEBUG_PRINTER, fmt, ap);
    va_end(ap);
    return ret;
}

STATIC void debug_print_strn(void *env, const char *str, size_t len) {
    (void) env;

    for (size_t i=0; i<len; i++) {

        if (trace_debug_buffer_len == sizeof(trace_debug_buffer) - 1 || str[i] == '\n') {
            // flush
            Trace(2, trace_debug_buffer);
            memset(trace_debug_buffer, 0, sizeof(trace_debug_buffer));
            trace_debug_buffer_len = 0;
        }

        if (str[i] != '\n') trace_debug_buffer[trace_debug_buffer_len++] = str[i];

    }
}

const mp_print_t mp_debug_print = {NULL, debug_print_strn};

#endif


