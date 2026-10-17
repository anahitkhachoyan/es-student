#include "stdio-text-protocol/stdio-text-protocol.h"
#include <stdio.h>
#include "pico/stdlib.h"

static char line_buf[STDIO_TEXT_PROTOCOL_LINE_SIZE];
static uint32_t line_len = 0;
static command_t current_command;

void stdio_text_protocol_init(void)
{
    line_len = 0;
    current_command.name = NULL;
    current_command.argc = 0;
    current_command.truncated = false;
}

const command_t *stdio_text_protocol_handle(void)
{
    int symbol = getchar_timeout_us(0);

    if (symbol == PICO_ERROR_TIMEOUT)
    {
        return NULL;
    }

    // Обработка Backspace (0x08 или 0x7f)
    if (symbol == 0x08 || symbol == 0x7f)
    {
        if (line_len > 0)
        {
            line_len--;
            printf("\b \b");
        }
        return NULL;
    }

    // Конец строки (Enter: \r или \n)
    if (symbol == '\r' || symbol == '\n')
    {
        putchar('\n');
        line_buf[line_len] = '\0';

        // Если строка пустая — просто сбрасываем и ничего не возвращаем
        if (line_len == 0)
        {
            return NULL;
        }

        // Проверяем флаг truncated (если в процессе ввода строка переполнилась)
        bool is_truncated = current_command.truncated;

        // Сбрасываем буфер для следующей строки
        line_len = 0;
        current_command.truncated = false;

        if (is_truncated)
        {
            // ИСПРАВЛЕНО: используем реальное имя из буфера, а не пустую строку
            current_command.name = line_buf;
            current_command.argc = 0;
            return &current_command;
        }

        // Парсинг строки на слова (in-place разделение пробелами)
        char *p = line_buf;
        
        // Пропускаем начальные пробелы
        while (*p == ' ' || *p == '\t')
        {
            p++;
        }

        if (*p == '\0')
        {
            return NULL; // Строка состояла только из пробелов
        }

        current_command.name = p;
        current_command.argc = 0;

        // Ищем конец имени команды
        while (*p != '\0' && *p != ' ' && *p != '\t')
        {
            p++;
        }

        if (*p != '\0')
        {
            *p = '\0'; // Заменяем пробел на нуль-терминатор
            p++;

            // Разбираем аргументы
            while (*p != '\0' && current_command.argc < STDIO_TEXT_PROTOCOL_MAX_ARGS)
            {
                // Пропускаем пробелы между аргументами
                while (*p == ' ' || *p == '\t')
                {
                    p++;
                }

                if (*p == '\0')
                {
                    break;
                }

                current_command.argv[current_command.argc++] = p;

                // Ищем конец текущего аргумента
                while (*p != '\0' && *p != ' ' && *p != '\t')
                {
                    p++;
                }

                if (*p != '\0')
                {
                    *p = '\0';
                    p++;
                }
            }

            // Если остались непрочитанные слова (аргументов больше, чем MAX_ARGS)
            while (*p != '\0')
            {
                if (*p != ' ' && *p != '\t')
                {
                    current_command.truncated = true;
                    break;
                }
                p++;
            }
        }

        return &current_command;
    }

    // Печатный символ
    if (line_len + 1 < STDIO_TEXT_PROTOCOL_LINE_SIZE)
    {
        line_buf[line_len++] = (char)symbol;
        putchar(symbol);
    }
    else
    {
        // Буфер переполнен
        current_command.truncated = true;
    }

    return NULL;
}