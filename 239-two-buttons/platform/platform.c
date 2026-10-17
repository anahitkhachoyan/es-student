#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifndef LED_PIN
#define LED_PIN 25
#endif

#ifndef BUTTON_1_PIN
#define BUTTON_1_PIN 15
#endif

#ifndef BUTTON_2_PIN
#define BUTTON_2_PIN 14
#endif

// Вывод каждой кнопки, индекс — platform_button_t
static const uint button_pins[PLATFORM_BUTTON_COUNT] = {
    BUTTON_1_PIN,
    BUTTON_2_PIN,
};

void platform_init(void)
{
    // Инициализация стандартного ввода-вывода (stdio)
    stdio_init_all();

    // Инициализация светодиода
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Инициализация кнопок с подтяжкой к питанию (pull-up)
    for (int i = 0; i < PLATFORM_BUTTON_COUNT; i++)
    {
        gpio_init(button_pins[i]);
        gpio_set_dir(button_pins[i], GPIO_IN);
        gpio_pull_up(button_pins[i]);
    }
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on ? 1 : 0);
}

bool platform_button_read(platform_button_t button)
{
    if ((int)button < 0 || button >= PLATFORM_BUTTON_COUNT)
    {
        return false;
    }
    // Кнопка замыкает пин на землю при нажатии (низкий уровень = 0 -> возвращаем true)
    return (gpio_get(button_pins[button]) == 0);
}