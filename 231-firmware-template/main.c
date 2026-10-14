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

    profiling_stopwatch_init(STOPWATCH_LOOP, "loop");
    profiling_stopwatch_init(STOPWATCH_COMMAND, "command");
    profiling_stopwatch_init(STOPWATCH_LED, "led");
    profiling_stopwatch_init(STOPWATCH_BUTTON, "button");
    profiling_stopwatch_init(STOPWATCH_PI, "pi");

    stdio_text_protocol_init();
    
    led_task_init(platform_led_set);
    button_task_init(platform_button_read, led_task_next_state);
    pi_task_init();
    
    // Инициализируем API и прокидываем системное время через безопасный колбэк
    api_init(systime_ms);

    LOG_INF("Starting %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    uint64_t last_blink_ms = 0;

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        // Таймер мигания светодиода управляется на верхнем уровне (в main.c)
        uint64_t now_ms = systime_ms();
        if (led_task_get_state() == LED_STATE_BLINK)
        {
            uint32_t period = led_task_get_period_ms();
            if (period > 0 && (now_ms - last_blink_ms >= (period / 2)))
            {
                last_blink_ms = now_ms;
                led_task_toggle(); // Переключаем состояние светодиода
            }
        }

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