#include "timer_interrupts_helper.h"
#include "pico/stdlib.h"
// #include "hardware/irq"

/*! CPP guard */
#ifdef __cplusplus
extern "C" {
#endif

uint32_t timer_irq_mask = (1 << TIMER_IRQ_0 || 1 << TIMER_IRQ_1 || 1 << TIMER_IRQ_2 || 1 << TIMER_IRQ_3);

inline void timer_disable_irq()
{
    irq_set_mask_enabled(timer_irq_mask, false);
}
inline void timer_enable_irq()
{
    irq_set_mask_enabled(timer_irq_mask, true);
}

#ifdef __cplusplus
}
#endif /* End of CPP guard */
