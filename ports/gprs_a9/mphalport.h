#ifndef __MPHALPORT_H
#define __MPHALPORT_H

#include "stdint.h"
#include "stdbool.h"
#include "lib/utils/interrupt_char.h"
#include "time.h"

void mp_hal_set_interrupt_char(int c);

bool UartInit();


uint32_t mp_hal_ticks_ms(void);
uint32_t mp_hal_ticks_us(void);
void mp_hal_delay_ms(uint32_t ms);
void mp_hal_delay_us(uint32_t us);
__attribute__((always_inline)) static inline uint32_t mp_hal_ticks_cpu(void) {
  return clock();
}


#endif

