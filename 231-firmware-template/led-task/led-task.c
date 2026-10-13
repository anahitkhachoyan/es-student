#include "led-task.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Пин встроенного светодиода на Raspberry Pi Pico обычно GPIO 25
#ifndef LED_PIN
#define LED_PIN 25
#endif

static led_state_t current_state = LED_STATE_BLINK;
static uint32_t period_ms = 1000;
static uint64_t last_toggle_us = 0;
static bool led_is_on = false;

static void set_gpio_output(bool on)
{
    led_is_on = on;
    gpio_put(LED_PIN, on ? 1 : 0);
}

// Проверка периода по сетке без накопления ошибки
static bool period_elapsed(uint64_t *last_us, uint64_t period_us)
{
    uint64_t now_us = time_us_64();
    if (now_us - *last_us >= period_us)
    {
        *last_us += period_us;
        return true;
    }
    return false;
}

void led_task_init(void)
{
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    period_ms = 1000;
    last_toggle_us = time_us_64();
    led_task_set_state(LED_STATE_BLINK);
}

void led_task_set_state(led_state_t state)
{
    current_state = state;
    uint64_t now_us = time_us_64();

    switch (current_state)
    {
    case LED_STATE_OFF:
        set_gpio_output(false);
        break;
    case LED_STATE_ON:
        set_gpio_output(true);
        break;
    case LED_STATE_BLINK:
        // Устанавливаем метку в прошлое на полупериод, чтобы переключиться сразу
        last_toggle_us = now_us - (period_ms * 1000ULL / 2ULL);
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

void led_task_handle(void)
{
    switch (current_state)
    {
    case LED_STATE_OFF:
    case LED_STATE_ON:
        // В этих состояниях задача ничего в цикле не делает, светодиод управляется при входе
        break;

    case LED_STATE_BLINK:
        // Переключаем каждые полпериода
        if (period_elapsed(&last_toggle_us, (uint64_t)(period_ms * 1000ULL / 2ULL)))
        {
            set_gpio_output(!led_is_on);
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