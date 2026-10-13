#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "pico/stdlib.h"
#include "device.h"
#include "profiling.h"
#include "led.h"

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

// Функция расчёта числа Пи по ряду Лейбница
static double calc_pi_func(uint32_t terms)
{
    double sum = 0.0;
    double sign = 1.0;

    for (uint32_t k = 0; k < terms; k++)
    {
        sum += sign / (2.0 * k + 1.0);
        sign = -sign;
    }

    return sum * 4.0;
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

static void cmd_calc_pi(const command_t *cmd)
{
    uint32_t terms = 1000000; // значение по умолчанию, если аргумент не передан

    if (cmd->argc > 1)
    {
        printf("error: usage calc_pi [terms]\n");
        return;
    }

    if (cmd->argc == 1)
    {
        if (strcmp(cmd->argv[0], "0") == 0)
        {
            printf("error: terms must be positive\n"); // или соответствующее требование из Приложения Б
            return;
        }

        if (!parse_u32(cmd->argv[0], &terms) || terms == 0)
        {
            printf("error: usage calc_pi [terms]\n");
            return;
        }
    }

    volatile double pi_result;
    uint64_t start_us = time_us_64();
    pi_result = calc_pi_func(terms);
    uint64_t spent_us = time_us_64() - start_us;

    printf("pi: %.9f\n", pi_result);
    printf("time: %llu ms\n", spent_us / 1000);
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

// Описание структуры команды в таблице API
typedef struct {
    const char *name;
    const char *help;
    void (*callback)(const command_t *cmd);
} api_command_t;

static const api_command_t commands[] = {
    { "info",            "device passport",                     cmd_info },
    { "uptime",          "time since reset",                    cmd_uptime },
    { "calc_pi",         "calculate pi: calc_pi [terms]",       cmd_calc_pi },
    { "main_time_exec",  "superloop iteration: average and maximum", cmd_main_time_exec },
    { "main_time_reset", "reset maximum iteration",             cmd_main_time_reset },
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