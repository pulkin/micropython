#include "mphalport.h"

#define WAIT_UNTIL(condition, timeout_ms, step_ms, raise) if (timeout_ms) do {uint32_t __time = mp_hal_ticks_ms(); while (mp_hal_ticks_ms() - __time < (timeout_ms) && !(condition)) mp_hal_delay_ms(step_ms); if (!(condition)) {raise;}} while (0)

