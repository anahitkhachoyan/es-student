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

    // При выключенном профилировании эти вызовы — пустые макросы
    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    stdio_text_protocol_init();

    led_task_init(platform_led_set);
    // Что делает нажатие каждой кнопки, решает main.c
    button_task_init(PLATFORM_BUTTON_1, led_task_next_state);
    button_task_init(PLATFORM_BUTTON_2, led_task_next_period);
    pi_task_init();

    api_init(systime_ms);

    LOG_INF("Starting %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        uint64_t now_us = systime_us();

        profiling_start(STOPWATCH_LED);
        led_task_handle(now_us);
        profiling_stop(STOPWATCH_LED);

        profiling_start(STOPWATCH_BUTTON);
        for (uint32_t b = 0; b < PLATFORM_BUTTON_COUNT; b++)
        {
            button_task_handle(b, platform_button_read((platform_button_t)b), now_us);
        }
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

    return 0;
}