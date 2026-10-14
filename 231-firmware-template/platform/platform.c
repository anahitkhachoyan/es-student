#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#ifndef LED_PIN
#define LED_PIN 25
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

void platform_init(void)
{
    // Инициализация стандартного ввода-вывода (stdio)
    stdio_init_all();

    // Инициализация светодиода
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);

    // Инициализация кнопки с подтяжкой к питанию (pull-up)
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on ? 1 : 0);
}

bool platform_button_read(void)
{
    // Кнопка замыкает пин на землю при нажатии (низкий уровень = 0 -> возвращаем true)
    return (gpio_get(BUTTON_PIN) == 0);
}