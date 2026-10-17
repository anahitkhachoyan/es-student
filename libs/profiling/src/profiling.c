#include "profiling/profiling.h"

#if PROFILING_ENABLED

#include <string.h>
#include "systime/systime.h"

typedef struct {
    const char *name;
    bool active;
    uint64_t start_us;
    uint32_t count;
    uint64_t sum_us;
    uint32_t max_us;
} stopwatch_t;

static stopwatch_t stopwatches[PROFILING_MAX_STOPWATCHES];

void profiling_stopwatch_init(uint32_t id, const char *name)
{
    if (id < PROFILING_MAX_STOPWATCHES)
    {
        stopwatches[id].name = name;
        stopwatches[id].active = true;
        stopwatches[id].start_us = 0;
        stopwatches[id].count = 0;
        stopwatches[id].sum_us = 0;
        stopwatches[id].max_us = 0;
    }
}

void profiling_start(uint32_t id)
{
    if (id < PROFILING_MAX_STOPWATCHES && stopwatches[id].active)
    {
        stopwatches[id].start_us = systime_us();
    }
}

void profiling_stop(uint32_t id)
{
    if (id < PROFILING_MAX_STOPWATCHES && stopwatches[id].active)
    {
        uint64_t now = systime_us();
        uint64_t elapsed = now - stopwatches[id].start_us;

        stopwatches[id].count++;
        stopwatches[id].sum_us += elapsed;
        if ((uint32_t)elapsed > stopwatches[id].max_us)
        {
            stopwatches[id].max_us = (uint32_t)elapsed;
        }
    }
}

bool profiling_get(uint32_t id, profiling_result_t *result)
{
    if (id >= PROFILING_MAX_STOPWATCHES || !stopwatches[id].active || result == NULL)
    {
        return false;
    }

    result->name = stopwatches[id].name;
    result->count = stopwatches[id].count;
    if (stopwatches[id].count > 0)
    {
        result->mean_us = (double)stopwatches[id].sum_us / (double)stopwatches[id].count;
    }
    else
    {
        result->mean_us = 0.0;
    }
    result->max_us = stopwatches[id].max_us;

    return true;
}

void profiling_reset(void)
{
    for (int i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (stopwatches[i].active)
        {
            // Обнуляем только накопленные результаты.
            // start_us не трогаем: секундомеры loop и command в момент сброса
            // уже запущены, и их stop после сброса должен считать
            // от настоящего времени старта, а не от нуля.
            stopwatches[i].count = 0;
            stopwatches[i].sum_us = 0;
            stopwatches[i].max_us = 0;
        }
    }
}

#endif // PROFILING_ENABLED