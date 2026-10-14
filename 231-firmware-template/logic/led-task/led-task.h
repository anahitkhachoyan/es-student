#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINK
} led_state_t;

void led_task_init(void (*led_set_cb)(bool));
void led_task_handle(void);
void led_task_set_state(led_state_t state);
led_state_t led_task_get_state(void);
void led_task_set_period_ms(uint32_t period_ms);
uint32_t led_task_get_period_ms(void); // Исправлено: без аргумента
void led_task_next_state(void);
void led_task_get_status_str(char *buf, size_t len);