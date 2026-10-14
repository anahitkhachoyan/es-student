#include "led-task.h"
#include <stdio.h>
#include <string.h>

static led_state_t current_state = LED_STATE_BLINK;
static uint32_t period_ms = 1000;
static bool led_is_on = false;
static led_task_output_t led_output = NULL;

void led_task_init(led_task_output_t output_cb)
{
    led_output = output_cb;
    period_ms = 1000;
    current_state = LED_STATE_BLINK;
    led_is_on = false;
}

void led_task_set_state(led_state_t state)
{
    current_state = state;
    if (led_output != NULL)
    {
        if (state == LED_STATE_OFF)
        {
            led_is_on = false;
            led_output(false);
        }
        else if (state == LED_STATE_ON)
        {
            led_is_on = true;
            led_output(true);
        }
    }
}

led_state_t led_task_get_state(void)
{
    return current_state;
}

void led_task_set_period_ms(uint32_t new_period_ms)
{
    period_ms = new_period_ms;
}

uint32_t led_task_get_period_ms(void)
{
    return period_ms;
}

void led_task_get_status_str(char *buf, size_t len)
{
    const char *state_str = "off";
    if (current_state == LED_STATE_ON)
    {
        state_str = "on";
    }
    else if (current_state == LED_STATE_BLINK)
    {
        state_str = "blink";
    }
    snprintf(buf, len, "led: %s, period %u ms", state_str, (unsigned)period_ms);
}

// Функция для внешнего управления миганием из main
void led_task_toggle(void)
{
    if (current_state == LED_STATE_BLINK && led_output != NULL)
    {
        led_is_on = !led_is_on;
        led_output(led_is_on);
    }
}

void led_task_handle(void)
{
    // Никаких часов внутри логики! Временем управляет main.c
}

void led_task_next_state(void)
{
    switch (current_state)
    {
    case LED_STATE_OFF:
        led_task_set_state(LED_STATE_ON);
        break;
    case LED_STATE_ON:
        led_task_set_state(LED_STATE_BLINK);
        break;
    case LED_STATE_BLINK: 
        led_task_set_state(LED_STATE_OFF);
        break;
    }
}