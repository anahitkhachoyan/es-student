#pragma once
#include <stdint.h>
#include <stdbool.h>

// Сколько кнопок задача может держать одновременно
#define BUTTON_TASK_MAX 4

typedef void (*button_task_callback_t)(void);

// Запускает кнопку с номером button: on_press вызывается при каждом нажатии
void button_task_init(uint32_t button, button_task_callback_t on_press);

// Один шаг машины кнопки button: состояние ножки и текущее время
void button_task_handle(uint32_t button, bool is_low, uint64_t now_us);

bool button_task_is_pressed(uint32_t button);
uint32_t button_task_get_press_count(uint32_t button);

// Сколько кнопок запущено: наибольший номер из button_task_init() плюс один
uint32_t button_task_get_count(void);