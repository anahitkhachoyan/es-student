#pragma once
#include <stdint.h>
#include <stdbool.h>

// Сколько кнопок задача может держать одновременно
#define BUTTON_TASK_MAX 4

typedef void (*button_task_callback_t)(void);

// Запускает кнопку с номером button.
// Если on_long_press задана, on_press сообщает о коротком нажатии при отпускании,
// а on_long_press — один раз за нажатие, пока кнопку держат дольше 1 с.
// Если on_long_press — NULL, on_press сообщает о нажатии сразу.
void button_task_init(uint32_t button, button_task_callback_t on_press, button_task_callback_t on_long_press);

// Один шаг машины кнопки button: состояние ножки. Время задача берёт у systime сама
void button_task_handle(uint32_t button, bool is_low);

bool button_task_is_pressed(uint32_t button);
uint32_t button_task_get_press_count(uint32_t button);

// Сколько кнопок запущено: наибольший номер при запуске плюс один
uint32_t button_task_get_count(void);