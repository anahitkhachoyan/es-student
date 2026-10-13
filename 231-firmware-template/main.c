#include <stdio.h>
#include "pico/stdlib.h"
#include "log.h"
#include "device.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

LOG_INF("Starting %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

enum {
    STOPWATCH_LOOP,
    STOPWATCH_COMMAND,
    STOPWATCH_LED,
    STOPWATCH_BUTTON,
    STOPWATCH_PI
};

int main()
{
    stdio_init_all();

    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    stdio_text_protocol_init();
    led_task_init();
    button_task_init();
    pi_task_init();

    LOG_INF("%s %s\n", DEVICE_NAME, FIRMWARE_VERSION);

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        profiling_start(STOPWATCH_LED);
        led_task_handle();
        profiling_stop(STOPWATCH_LED);

        profiling_start(STOPWATCH_BUTTON);
        button_task_handle();
        profiling_stop(STOPWATCH_BUTTON);

        profiling_start(STOPWATCH_PI);
        pi_task_handle();
        profiling_stop(STOPWATCH_PI);

        profiling_start(STOPWATCH_COMMAND);
        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
        profiling_stop(STOPWATCH_COMMAND);

        profiling_stop(STOPWATCH_LOOP);
    }
}