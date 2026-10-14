#include "api-commands.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "device/device.h"
#include "profiling/profiling.h"
#include "systime/systime.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"

// Вспомогательная функция для безопасного разбора 32-битного числа без знака из строки
static bool parse_u32(const char *str, uint32_t *out_value)
{
    if (str == NULL || *str == '\0')
    {
        return false;
    }

    char *endptr;
    errno = 0;
    unsigned long val = strtoul(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0')
    {
        return false;
    }

    *out_value = (uint32_t)val;
    return true;
}

// Печать текущего статуса светодиода
static void print_led_status(void)
{
    char buf[64];
    led_task_get_status_str(buf, sizeof(buf));
    printf("%s\n", buf);
}

// Обработчики команд
static void cmd_info(const command_t *cmd)
{
    (void)cmd;
    device_info_t info;
    device_get_info(&info);

    printf("project: %s\n", info.project);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("pico-sdk: %s\n", info.sdk_version);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
}

static void cmd_uptime(const command_t *cmd)
{
    (void)cmd;
    printf("uptime: %llu ms\n", systime_ms());
}

static void cmd_profiling(const command_t *cmd)
{
    (void)cmd;
#if PROFILING_ENABLED
    profiling_result_t res;
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (profiling_get(i, &res))
        {
            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   res.name,
                   (unsigned)res.count,
                   res.mean_us,
                   (unsigned)res.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *cmd)
{
    (void)cmd;
#if PROFILING_ENABLED
    profiling_reset();
    printf("profiling reset\n");
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_led_enable(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_ON);
    print_led_status();
}

static void cmd_led_disable(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_OFF);
    print_led_status();
}

static void cmd_led_blink(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_BLINK);
    print_led_status();
}

static void cmd_led_period(const command_t *cmd)
{
    if (cmd->argc != 1)
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    uint32_t new_period = 0;
    if (!parse_u32(cmd->argv[0], &new_period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (new_period == 0)
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    led_task_set_period_ms(new_period);
    if (led_task_get_state() == LED_STATE_BLINK)
    {
        led_task_set_state(LED_STATE_BLINK);
    }
    print_led_status();
}

static void cmd_button(const command_t *cmd)
{
    (void)cmd;
    const char *status_str = button_task_is_pressed() ? "pressed" : "released";
    printf("button: %s, presses %u\n", status_str, (unsigned)button_task_get_press_count());
}

static void cmd_pi_start(const command_t *cmd)
{
    uint32_t terms = 1000000;

    if (cmd->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (cmd->argc == 1)
    {
        if (strcmp(cmd->argv[0], "0") == 0)
        {
            printf("error: terms must be positive\n");
            return;
        }

        if (!parse_u32(cmd->argv[0], &terms) || terms == 0)
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void cmd_pi(const command_t *cmd)
{
    (void)cmd;
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
        uint64_t dur_ms = 0;
        pi_task_get_result(&pi_val, NULL, &dur_ms);
        printf("pi: %.9f in %llu ms\n", pi_val, dur_ms);
    }
}

// Таблица команд прибора
const api_command_t api_commands[] = {
    { "info",            "device passport",                        cmd_info },
    { "uptime",          "time since reset",                       cmd_uptime },
    { "profiling",       "profiling results for all stopwatches",    cmd_profiling },
    { "profiling_reset", "reset profiling stopwatches",              cmd_profiling_reset },
    { "led_enable",      "turn on led",                            cmd_led_enable },
    { "led_disable",     "turn off led",                           cmd_led_disable },
    { "led_blink",       "blink led",                              cmd_led_blink },
    { "led_period",      "set blink period: led_period <period_ms>", cmd_led_period },
    { "button",          "button status and press count",          cmd_button },
    { "pi_start",        "start pi calculation: pi_start [terms]",   cmd_pi_start },
    { "pi",              "get pi calculation status or result",      cmd_pi },
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);