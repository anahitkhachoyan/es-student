#include "systime/systime.h"
#include "pico/stdlib.h"

uint64_t systime_us(void)
{
    return to_us_since_boot(get_absolute_time());
}

uint64_t systime_ms(void)
{
    return to_ms_since_boot(get_absolute_time());
}

bool systime_period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t current = systime_us();
    if (current - *last_us >= period_us)
    {
        *last_us = current;
        return true;
    }
    return false;
}