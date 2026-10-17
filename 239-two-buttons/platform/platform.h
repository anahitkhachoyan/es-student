#pragma once

#include <stdbool.h>

// Кнопки платы. Сколько их и на каких выводах, знает только platform.c
typedef enum {
    PLATFORM_BUTTON_1,
    PLATFORM_BUTTON_2,
    PLATFORM_BUTTON_COUNT
} platform_button_t;

// Запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

// Управление светодиодом
void platform_led_set(bool on);

// Чтение состояния кнопки (нажата ли прямо сейчас, без учета дребезга)
bool platform_button_read(platform_button_t button);