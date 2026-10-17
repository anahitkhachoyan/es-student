#include "api.h"
#include "api-commands.h"
#include <stdio.h>
#include <string.h>

static api_uptime_cb_t s_uptime_cb = NULL;

void api_init(api_uptime_cb_t uptime_cb)
{
    s_uptime_cb = uptime_cb;
}

uint64_t api_get_uptime(void)
{
    if (s_uptime_cb != NULL)
    {
        return s_uptime_cb();
    }
    return 0;
}

void api_handle(const command_t *command)
{
    // 1. Проверка на усечение (truncated)
    if (command->truncated)
    {
        printf("error: command longer than 63 characters or 4 arguments\n");
        return;
    }

    // 2. Обработка команды help
    if (strcmp(command->name, "help") == 0)
    {
        printf("help             list of commands\n");
        for (uint32_t i = 0; i < api_command_count; i++)
        {
            printf("%-16s %s\n", api_commands[i].name, api_commands[i].help);
        }
        return;
    }

    // 3. Поиск и выполнение команды в таблице
    for (uint32_t i = 0; i < api_command_count; i++)
    {
        if (strcmp(command->name, api_commands[i].name) == 0)
        {
            if (api_commands[i].callback != NULL)
            {
                api_commands[i].callback(command);
            }
            return;
        }
    }

    // 4. Команда не найдена (строго с угловыми скобками)
    printf("error: unknown command '<%s>', try help\n", command->name);
}