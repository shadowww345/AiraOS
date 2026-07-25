#ifndef PIT_H
#define PIT_H

#include <kernel.h>

#define PIT_HZ 1000

void pit_init();

uint32_t get_ticks_ms();

void sleep_ms(uint32_t ms);

#endif
