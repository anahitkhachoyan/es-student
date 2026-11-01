#include "platform/platform.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"

#ifndef LED_PIN
#define LED_PIN 25
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 15
#endif

// вывод, через резистор заряжающий конденсатор RC-цепи
#define CHARGE_PIN 22

// вход АЦП: вывод GP26, канал 0
#define ADC_PIN 26
#define ADC_CHANNEL 0

// встроенный датчик температуры — канал 4
#define ADC_TEMP_CHANNEL 4

// опорное напряжение АЦП на плате Pico и число уровней 12-разрядного кода
#define ADC_VREF_V 3.3f
#define ADC_LEVELS 4096

// датчик температуры по RP2040 Datasheet, раздел 4.9.5:
// при 27 °C на нём 0,706 В, и на каждый градус напряжение падает на 0,001721 В
#define TEMP_SENSOR_V_AT_27C 0.706f
#define TEMP_SENSOR_V_PER_C 0.001721f

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

    // Вывод заряда RC-цепи: выход, на нём ноль, чтобы конденсатор оставался разряженным
    gpio_init(CHARGE_PIN);
    gpio_set_dir(CHARGE_PIN, GPIO_OUT);
    gpio_put(CHARGE_PIN, 0);

    // АЦП: запустить модуль и отдать вывод GP26 аналоговому входу
    adc_init();
    adc_gpio_init(ADC_PIN);

    // питание встроенного датчика температуры включается отдельно
    adc_set_temp_sensor_enabled(true);
}

void platform_led_set(bool on)
{
    gpio_put(LED_PIN, on ? 1 : 0);
}

void platform_charge_set(bool on)
{
    gpio_put(CHARGE_PIN, on ? 1 : 0);
}

bool platform_button_read(void)
{
    // Кнопка замыкает пин на землю при нажатии (низкий уровень = 0 -> возвращаем true)
    return (gpio_get(BUTTON_PIN) == 0);
}

// Каждая функция сама выбирает свой канал перед отсчётом,
// поэтому get_voltage и get_temp можно вызывать в любом порядке.

float platform_voltage_read(void)
{
    adc_select_input(ADC_CHANNEL);
    return adc_read() * ADC_VREF_V / ADC_LEVELS;
}

float platform_temp_read(void)
{
    adc_select_input(ADC_TEMP_CHANNEL);
    float volts = adc_read() * ADC_VREF_V / ADC_LEVELS;
    return 27.0f - (volts - TEMP_SENSOR_V_AT_27C) / TEMP_SENSOR_V_PER_C;
}