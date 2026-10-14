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
#include "api-commands.h"
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

// Переменные для измерения времени выполнения задачи Pi на верхнем уровне
static uint64_t pi_start_time_us = 0;
static uint64_t pi_duration_ms = 0;
static pi_state_t last_pi_state = PI_STATE_NOT_STARTED;

// Обработчик команды uptime на уровне main/платформы
static void handle_uptime_command_if_needed(const command_t *command)
{
    if (strcmp(command->name, "uptime") == 0)
    {
        printf("uptime: %llu ms\n", systime_ms());
    }
}

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

    LOG_INF("Starting %s v%s\n", FIRMWARE_NAME, FIRMWARE_VERSION);

    while (1)
    {
        profiling_start(STOPWATCH_LOOP);

        // Отслеживание состояния pi_task для замера времени выполнения на верхнем уровне
        pi_state_t current_pi_state = pi_task_get_state();
        if (last_pi_state != PI_STATE_RUNNING && current_pi_state == PI_STATE_RUNNING)
        {
            pi_start_time_us = systime_us();
        }
        else if (last_pi_state == PI_STATE_RUNNING && current_pi_state == PI_STATE_READY)
        {
            uint64_t end_time_us = systime_us();
            pi_duration_ms = (end_time_us - pi_start_time_us) / 1000ULL;
        }
        last_pi_state = current_pi_state;

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
            if (strcmp(command->name, "uptime") == 0)
            {
                handle_uptime_command_if_needed(command);
            }
            else if (strcmp(command->name, "pi") == 0)
            {
                // Переопределяем вывод команды pi на верхнем уровне с учетом замеренного времени
                pi_state_t state = pi_task_get_state();
                if (state == PI_STATE_NOT_STARTED)
                {
                    printf("pi: not started\n");
                }
                else if (state == PI_STATE_RUNNING)
                {
                    printf("pi: running\n");
                }
                else if (state == PI_STATE_READY)
                {
                    double pi_val = 0.0;
                    uint32_t terms = 0;
                    pi_task_get_result(&pi_val, &terms, NULL);
                    printf("pi: %.9f in %llu ms\n", pi_val, pi_duration_ms);
                }
            }
            else
            {
                api_handle(command);
            }
        }
        profiling_stop(STOPWATCH_COMMAND);

        profiling_stop(STOPWATCH_LOOP);
    }
}