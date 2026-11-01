#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef bool (*button_task_read_t)(void);
typedef void (*button_task_callback_t)(void);

void button_task_init(button_task_read_t read_cb, button_task_callback_t on_press);
void button_task_handle(bool is_low, uint64_t now_us); // Принимает состояние и время
bool button_task_is_pressed(void);
uint32_t button_task_get_press_count(void);