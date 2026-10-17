#include "led-task.h"
#include <stdio.h>
#include <string.h>

static led_state_t current_state = LED_STATE_BLINK;
static uint32_t period_ms = 1000;
static uint64_t last_toggle_us = 0;
static bool led_is_on = false;
static led_task_output_t led_output = NULL;

static void set_led_output(bool on)
{
    led_is_on = on;
    if (led_output != NULL)
    {
        led_output(on);
    }
}

static bool period_elapsed(uint64_t *last_us, uint64_t period_us, uint64_t now_us)
{
    if (now_us - *last_us >= period_us)
    {
        *last_us += period_us;
        return true;
    }
    return false;
}

void led_task_init(led_task_output_t output_cb)
{
    led_output = output_cb;
    period_ms = 1000;
    last_toggle_us = 0;
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    current_state = state;
    switch (current_state)
    {
    case LED_STATE_OFF:
        set_led_output(false);
        break;
    case LED_STATE_ON:
        set_led_output(true);
        break;
    case LED_STATE_BLINK:
        break;
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

// Круг периодов мигания для кнопки: 250 -> 500 -> 1000 -> 250 мс
static const uint32_t period_circle_ms[] = { 250, 500, 1000 };

void led_task_next_period(void)
{
    // Берём ближайший период из круга, который больше текущего;
    // если такого нет (или текущий вне круга и больше всех) — начинаем круг заново
    uint32_t next = period_circle_ms[0];
    for (size_t i = 0; i < sizeof(period_circle_ms) / sizeof(period_circle_ms[0]); i++)
    {
        if (period_ms < period_circle_ms[i])
        {
            next = period_circle_ms[i];
            break;
        }
    }
    led_task_set_period_ms(next);
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

void led_task_handle(uint64_t now_us)
{
    switch (current_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        break;

    case LED_STATE_BLINK:
        if (period_elapsed(&last_toggle_us, (uint64_t)(period_ms * 1000ULL / 2ULL), now_us))
        {
            set_led_output(!led_is_on);
        }
        break;
    }
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