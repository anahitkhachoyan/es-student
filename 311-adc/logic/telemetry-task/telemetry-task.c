#include "telemetry-task.h"
#include "platform/platform.h"
#include "systime/systime.h"
#include <stddef.h>

// самый короткий период, который задача принимает
#define TELEMETRY_MIN_PERIOD_MS 10

static telemetry_state_t state = TELEMETRY_STATE_IDLE;
static uint32_t period_ms = 0;
static uint64_t last_us = 0;
static telemetry_task_callback_t record_callback = NULL;

void telemetry_task_init(telemetry_task_callback_t on_record)
{
    record_callback = on_record;
    state = TELEMETRY_STATE_IDLE;
    period_ms = 0;
    last_us = 0;
}

bool telemetry_task_start(uint32_t new_period_ms)
{
    if (new_period_ms < TELEMETRY_MIN_PERIOD_MS)
    {
        return false;
    }

    period_ms = new_period_ms;
    // Метка на один период в прошлом: первая запись уйдёт на ближайшей итерации,
    // а следующие встанут на сетку времени от неё
    last_us = systime_us() - (uint64_t)period_ms * 1000ULL;
    state = TELEMETRY_STATE_RUN;
    return true;
}

void telemetry_task_stop(void)
{
    state = TELEMETRY_STATE_IDLE;
}

telemetry_state_t telemetry_task_get_state(void)
{
    return state;
}

uint32_t telemetry_task_get_period_ms(void)
{
    return period_ms;
}

void telemetry_task_handle(void)
{
    switch (state)
    {
    case TELEMETRY_STATE_IDLE:
        break;

    case TELEMETRY_STATE_RUN:
        if (systime_period_elapsed(&last_us, (uint64_t)period_ms * 1000ULL))
        {
            telemetry_record_t record;
            record.time_us = systime_us();
            record.voltage_v = platform_voltage_read();
            record.temp_c = platform_temp_read();
            if (record_callback != NULL)
            {
                record_callback(&record);
            }
        }
        break;
    }
}