#include <stdio.h>
#include <string.h>
#include "logging/log.h"
#include "device/device.h"
#include "firmware.h"
#include "profiling/profiling.h"
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "systime/systime.h"
#include "platform/platform.h"
#include "api/api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

enum {
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI
};

int main()
{
    platform_init();

#if PROFILING_ENABLED
    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");
#endif

    stdio_text_protocol_init();
    
    led_task_init(platform_led_set);
    button_task_init(platform_button_read, led_task_next_state);
    pi_task_init();
    
    api_init(systime_ms);

    LOG_INF("Starting %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    while (1)
    {
#if PROFILING_ENABLED
        profiling_start(STOPWATCH_LOOP);
#endif

        uint64_t now_us = systime_us();

#if PROFILING_ENABLED
        profiling_start(STOPWATCH_LED);
#endif
        led_task_handle(now_us);
#if PROFILING_ENABLED
        profiling_stop(STOPWATCH_LED);
#endif

#if PROFILING_ENABLED
        profiling_start(STOPWATCH_BUTTON);
#endif
        button_task_handle(platform_button_read(), now_us); 
#if PROFILING_ENABLED
        profiling_stop(STOPWATCH_BUTTON);
#endif

#if PROFILING_ENABLED
        profiling_start(STOPWATCH_PI);
#endif
        pi_task_handle();
#if PROFILING_ENABLED
        profiling_stop(STOPWATCH_PI);
#endif

#if PROFILING_ENABLED
        profiling_start(STOPWATCH_COMMAND);
#endif
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
#if PROFILING_ENABLED
        profiling_stop(STOPWATCH_COMMAND);
#endif

#if PROFILING_ENABLED
        profiling_stop(STOPWATCH_LOOP);
#endif
    }
    
    return 0;
}