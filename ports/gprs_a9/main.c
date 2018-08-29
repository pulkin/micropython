#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "lib/utils/pyexec.h"

#include "stdbool.h"
#include "api_os.h"
#include "api_event.h"
#include "api_debug.h"
#include "api_hal_pm.h"
#include "api_hal_uart.h"
#include "buffer.h"



#define AppMain_TASK_STACK_SIZE    (2048 * 2)
#define AppMain_TASK_PRIORITY      0
#define MICROPYTHON_TASK_STACK_SIZE     (2048 * 2)
#define MICROPYTHON_TASK_PRIORITY       1
#define UART_CIRCLE_FIFO_BUFFER_MAX_LENGTH 2048

HANDLE mainTaskHandle  = NULL;
HANDLE microPyTaskHandle = NULL;
Buffer_t fifoBuffer;
uint8_t  fifoBufferData[UART_CIRCLE_FIFO_BUFFER_MAX_LENGTH];

typedef enum
{
    MICROPY_EVENT_ID_UART_RECEIVED = 1, //param1: length, param2:data(uint8_t*)
    MICROPY_EVENT_ID_UART_MAX
}MicroPy_Event_ID_t;

typedef struct
{
    MicroPy_Event_ID_t id;
    uint32_t param1;
    uint8_t* pParam1;
}MicroPy_Event_t;







#if MICROPY_ENABLE_COMPILER
void do_str(const char *src, mp_parse_input_kind_t input_kind) {
    nlr_buf_t nlr;
    if (nlr_push(&nlr) == 0) {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(MP_QSTR__lt_stdin_gt_, src, strlen(src), 0);
        qstr source_name = lex->source_name;
        mp_parse_tree_t parse_tree = mp_parse(lex, input_kind);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
    } else {
        // uncaught exception
        mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    }
}
#endif

static char *stack_top;


void gc_collect(void) {
    // WARNING: This gc_collect implementation doesn't try to get root
    // pointers from CPU registers, and thus may function incorrectly.
    void *dummy;
    gc_collect_start();
    gc_collect_root(&dummy, ((mp_uint_t)stack_top - (mp_uint_t)&dummy) / sizeof(mp_uint_t));
    gc_collect_end();
    gc_dump_info();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename) {
    mp_raise_OSError(MP_ENOENT);
}

mp_import_stat_t mp_import_stat(const char *path) {
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val) {
    assert("nlr_jump_fail");
}

void NORETURN __fatal_error(const char *msg) {
    assert(msg);
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr) {
    printf("Assertion '%s' failed, at file %s:%d\n", expr, file, line);
    __fatal_error("Assertion failed");
}
#endif




bool mp_Init()
{
    int stack_dummy;
    stack_top = (char*)&stack_dummy;

    mp_init();
    pyexec_event_repl_init();
    return true;
}

bool UartInit()
{
    UART_Config_t config = {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity   = UART_PARITY_NONE,
        .rxCallback = NULL,
        .useEvent = true,
    };
    UART_Init(UART1,config);
    Buffer_Init(&fifoBuffer,fifoBufferData,sizeof(fifoBufferData));
}

void MicroPyEventDispatch(MicroPy_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case MICROPY_EVENT_ID_UART_RECEIVED:
        {
            uint8_t c;
            bool ret;

            for (;;) {
                if(!Buffer_Gets(&fifoBuffer,&c,1))
                    break;
                if (pyexec_event_repl_process_char((int)c)) {
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}


void MicroPyTask(VOID *pData)
{
    MicroPy_Event_t* event;

    UartInit();
    mp_Init();

    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            // PM_SetSysMinFreq(PM_SYS_FREQ_178M);//set back system min frequency to 178M or higher(/lower) value
            MicroPyEventDispatch(event);
            OS_Free(event->pParam1);
            // PM_SetSysMinFreq(PM_SYS_FREQ_32K);//release system freq to enter sleep mode to save power,
                                              //system remain runable but slower, and close eripheral not using
        }
    }
}
void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_POWER_ON:
            break;
        case API_EVENT_ID_NO_SIMCARD:
            break;
        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
            break;
        case API_EVENT_ID_UART_RECEIVED:
            Trace(1,"UART%d received:%d,%s",pEvent->param1,pEvent->param2,pEvent->pParam1);
            if(pEvent->param1 == UART1)
            {
                MicroPy_Event_t event;

                Buffer_Puts(&fifoBuffer,pEvent->pParam1,pEvent->param2);
                memset((void*)&event,0,sizeof(MicroPy_Event_t));
                event.id = MICROPY_EVENT_ID_UART_RECEIVED;
                event.param1 = (uint32_t)(pEvent->param2);
                OS_SendEvent(microPyTaskHandle,&event,OS_TIME_OUT_WAIT_FOREVER,0);
            }
            break;
        default:
            break;
    }
}


void AppMainTask(VOID *pData)
{
    API_Event_t* event=NULL;

    microPyTaskHandle = OS_CreateTask(MicroPyTask,
                                    NULL, NULL, MICROPYTHON_TASK_STACK_SIZE, MICROPYTHON_TASK_PRIORITY, 0, 0, "ohter Task");
    while(1)
    {
        if(OS_WaitEvent(mainTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            // PM_SetSysMinFreq(PM_SYS_FREQ_178M);//set back system min frequency to 178M or higher(/lower) value
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
            // PM_SetSysMinFreq(PM_SYS_FREQ_32K);//release system freq to enter sleep mode to save power,
                                              //system remain runable but slower, and close eripheral not using
        }
    }
}
int _Main(void)
{
    mainTaskHandle = OS_CreateTask(AppMainTask ,
                                   NULL, NULL, MICROPYTHON_TASK_PRIORITY, AppMain_TASK_PRIORITY, 0, 0, "init Task");
    OS_SetUserMainHandle(&mainTaskHandle);
    return 0;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    UART_Write(UART1,str,len);
}

// void mp_hal_stdout_tx_str(const char *str) {
//     mp_hal_stdout_tx_strn(str, strlen(str));
// }
