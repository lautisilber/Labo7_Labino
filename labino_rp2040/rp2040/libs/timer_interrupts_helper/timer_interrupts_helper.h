#ifndef TIMER_INTERRUPTS_HELPER_H
#define TIMER_INTERRUPTS_HELPER_H

#include "pico/stdlib.h"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

inline void timer_disable_irq();
inline void timer_enable_irq();

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* TIMER_INTERRUPTS_HELPER_H */