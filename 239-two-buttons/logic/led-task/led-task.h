#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum {
    LED_STATE_OFF,
    LED_STATE_ON,
    LED_STATE_BLINK
} led_state_t;

typedef void (*led_task_output_t)(bool);

void led_task_init(led_task_output_t output_cb);
void led_task_handle(uint64_t now_us); // Принимает готовое время аргументом
void led_task_set_state(led_state_t state);
led_state_t led_task_get_state(void);
void led_task_set_period_ms(uint32_t period_ms);
uint32_t led_task_get_period_ms(void);
void led_task_next_state(void);
void led_task_next_period(void);
void led_task_blink(void);
void led_task_get_status_str(char *buf, size_t len);