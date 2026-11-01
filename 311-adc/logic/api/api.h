#pragma once
#include <stdint.h>
#include "stdio-text-protocol/stdio-text-protocol.h"

// Тип колбэка для получения системного времени
typedef uint64_t (*api_uptime_cb_t)(void);

// Инициализация API с передачей внешней функции времени
void api_init(api_uptime_cb_t uptime_cb);

// Обработка команды
void api_handle(const command_t *cmd);

// Получение времени для внутренних нужд API (например, для uptime)
uint64_t api_get_uptime(void);