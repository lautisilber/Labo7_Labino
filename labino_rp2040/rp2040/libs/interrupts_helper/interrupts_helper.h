#ifndef TIMER_INTERRUPTS_HELPER_H
#define TIMER_INTERRUPTS_HELPER_H

#include <pico/types.h>
#include <hardware/sync.h>

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

// from https://www.makermatrix.com/blog/read-and-write-data-with-the-pi-pico-onboard-flash/
//
// uint32_t ints = save_and_disable_interrupts();
// code that is to be run atomically
// restore_interrupts (ints);

#ifdef __cplusplus
}
#endif /* End of CPP guard */

#endif /* TIMER_INTERRUPTS_HELPER_H */