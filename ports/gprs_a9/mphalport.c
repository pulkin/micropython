
#include "mpconfigport.h"

#include "time.h"

mp_uint_t mp_hal_ticks_ms(void)
{
    return (int)(clock()/CLOCKS_PER_MSEC);
}

void mp_hal_set_interrupt_char(int c) 
{


}
