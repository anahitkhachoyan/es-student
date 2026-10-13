#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "pico/stdlib.h"
#include "device.h"
#include "profiling.h"
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
    device_info();
}

static void cmd_uptime(const command_t *cmd)
{
    (void)cmd;
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}

static void cmd_main_time_exec(const command_t *cmd)
{
    (void)cmd;
    printf("iteration avg %.2f us, max %u us\n", profiling_avg_us(), (unsigned)profiling_max_us());
}

static void cmd_main_time_reset(const command_t *cmd)
{
    (void)cmd;
    profiling_reset_max();
    printf("max reset\n");
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
    uint32_t terms = 1000000; // значение по умолчанию

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

// Описание структуры команды в таблице API
typedef struct {
    const char *name;
    const char *help;
    void (*callback)(const command_t *cmd);
} api_command_t;

static const api_command_t commands[] = {
    { "info",            "device passport",                          cmd_info },
    { "uptime",          "time since reset",                         cmd_uptime },
    { "main_time_exec",  "superloop iteration: average and maximum", cmd_main_time_exec },
    { "main_time_reset", "reset maximum iteration",                  cmd_main_time_reset },
    { "led_enable",      "turn on led",                              cmd_led_enable },
    { "led_disable",     "turn off led",                             cmd_led_disable },
    { "led_blink",       "blink led",                                cmd_led_blink },
    { "led_period",      "set blink period: led_period <period_ms>", cmd_led_period },
    { "button",          "button status and press count",            cmd_button },
    { "pi_start",        "start pi calculation: pi_start [terms]",   cmd_pi_start },
    { "pi",              "get pi calculation status or result",      cmd_pi },
};

static const uint32_t command_count = sizeof(commands) / sizeof(commands[0]);

void api_handle(const command_t *command)
{
    // 1. Проверка на переполнение / превышение длины или аргументов
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    // 2. Обработка команды help
    if (strcmp(command->name, "help") == 0)
    {
        printf("help             list of commands\n");
        for (uint32_t i = 0; i < command_count; i++)
        {
            printf("%-16s %s\n", commands[i].name, commands[i].help);
        }
        return;
    }

    // 3. Поиск и выполнение команды в таблице
    for (uint32_t i = 0; i < command_count; i++)
    {
        if (strcmp(command->name, commands[i].name) == 0)
        {
            if (commands[i].callback != NULL)
            {
                commands[i].callback(command);
            }
            return;
        }
    }

    // 4. Команда не найдена
    printf("error: unknown command '%s', try help\n", command->name);
}