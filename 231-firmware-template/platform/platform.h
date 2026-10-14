#pragma once

#include <stdbool.h>

// Запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

// Управление светодиодом
void platform_led_set(bool on);

// Чтение состояния кнопки (нажата ли прямо сейчас, без учета дребезга)
bool platform_button_read(void);