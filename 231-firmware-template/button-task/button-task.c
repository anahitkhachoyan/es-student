#include "button-task.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "led-task.h"

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

// Порог подавления дребезга: 20 мс = 20000 мкс
#define DEBOUNCE_DELAY_US 20000ULL

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint32_t press_count = 0;
static uint64_t transition_time_us = 0;

void button_task_init(void)
{
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    button_state = BUTTON_STATE_RELEASED;
    press_count = 0;
    transition_time_us = time_us_64();
}

bool button_task_is_pressed(void)
{
    // Кнопка считается нажатой и в состоянии PRESSED, и пока идет подтверждение RELEASING
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(void)
{
    // Кнопка замыкает пин на землю при нажатии (низкий уровень = 0)
    bool is_low = (gpio_get(BUTTON_PIN) == 0);
    uint64_t now_us = time_us_64();

    switch (button_state)
    {
    case BUTTON_STATE_RELEASED:
        if (is_low)
        {
            transition_time_us = now_us;
            button_state = BUTTON_STATE_PRESSING;
        }
        break;

    case BUTTON_STATE_PRESSING:
        if (!is_low)
        {
            // Уровень вернулся назад — это дребезг/помеха, возвращаемся в отпущенное состояние
            button_state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - transition_time_us >= DEBOUNCE_DELAY_US)
        {
            // Продержалось 20 мс — нажатие подтверждено!
            press_count++;
            led_task_next_state();
            button_state = BUTTON_STATE_PRESSED;
        }
        break;

    case BUTTON_STATE_PRESSED:
        if (!is_low)
        {
            transition_time_us = now_us;
            button_state = BUTTON_STATE_RELEASING;
        }
        break;

    case BUTTON_STATE_RELEASING:
        if (is_low)
        {
            // Снова замкнуло — ложное отпускание, возвращаемся в нажатое состояние
            button_state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - transition_time_us >= DEBOUNCE_DELAY_US)
        {
            // Отпускание подтверждено 20 мс тишины
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}