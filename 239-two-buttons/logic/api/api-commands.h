#pragma once

#include <stdint.h>
#include "api.h"

// Описание структуры команды в таблице API
typedef struct {
    const char *name;
    const char *help;
    void (*callback)(const command_t *cmd);
} api_command_t;

extern const api_command_t api_commands[];
extern const uint32_t api_command_count;