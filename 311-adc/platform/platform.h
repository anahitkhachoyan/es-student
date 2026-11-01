#pragma once

#include <stdbool.h>

// Запускает стандартный ввод-вывод и настраивает выводы
void platform_init(void);

// Управление светодиодом
void platform_led_set(bool on);

// Чтение состояния кнопки (нажата ли прямо сейчас, без учета дребезга)
bool platform_button_read(void);

// Напряжение на входе АЦП, выводе GP26, в вольтах
float platform_voltage_read(void);

// Температура кристалла по встроенному датчику, градусы Цельсия
float platform_temp_read(void);

// Заряд RC-цепи: true — на цепь 3,3 В, false — ноль, цепь разряжается
void platform_charge_set(bool on);