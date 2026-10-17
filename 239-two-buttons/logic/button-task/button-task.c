#include "button-task.h"
#include "systime/systime.h"
#include <stddef.h>

#define DEBOUNCE_DELAY_US 20000
#define LONG_PRESS_US 1000000 // порог долгого нажатия: 1 с

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

// Состояние одной кнопки: у каждой кнопки своя независимая машина
typedef struct {
    button_state_t state;
    uint32_t press_count;
    uint64_t press_time_us;   // когда кнопку начали нажимать
    uint64_t release_time_us; // когда кнопку начали отпускать
    bool long_reported;       // о долгом нажатии уже сообщили
    button_task_callback_t press_callback;
    button_task_callback_t long_press_callback;
} button_t;

static button_t buttons[BUTTON_TASK_MAX];
static uint32_t button_count = 0;

void button_task_init(uint32_t button, button_task_callback_t on_press, button_task_callback_t on_long_press)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return;
    }

    buttons[button].state = BUTTON_STATE_RELEASED;
    buttons[button].press_count = 0;
    buttons[button].press_time_us = 0;
    buttons[button].release_time_us = 0;
    buttons[button].long_reported = false;
    buttons[button].press_callback = on_press;
    buttons[button].long_press_callback = on_long_press;

    if (button + 1 > button_count)
    {
        button_count = button + 1;
    }
}

uint32_t button_task_get_count(void)
{
    return button_count;
}

bool button_task_is_pressed(uint32_t button)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return false;
    }
    return (buttons[button].state == BUTTON_STATE_PRESSED ||
            buttons[button].state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(uint32_t button)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return 0;
    }
    return buttons[button].press_count;
}

void button_task_handle(uint32_t button, bool is_low)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return;
    }

    button_t *b = &buttons[button];
    uint64_t now_us = systime_us();

    switch (b->state)
    {
    case BUTTON_STATE_RELEASED:
        if (is_low)
        {
            b->press_time_us = now_us;
            b->state = BUTTON_STATE_PRESSING;
        }
        break;
    case BUTTON_STATE_PRESSING:
        if (!is_low)
        {
            b->state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - b->press_time_us >= DEBOUNCE_DELAY_US)
        {
            b->press_count++;
            b->long_reported = false;
            b->state = BUTTON_STATE_PRESSED;
            // Если у кнопки нет долгого действия, о нажатии сообщаем сразу.
            // Иначе ждём: пока не известно, короткое оно или долгое
            if (b->long_press_callback == NULL && b->press_callback != NULL)
            {
                b->press_callback();
            }
        }
        break;
    case BUTTON_STATE_PRESSED:
        if (!is_low)
        {
            b->release_time_us = now_us;
            b->state = BUTTON_STATE_RELEASING;
        }
        else if (!b->long_reported && now_us - b->press_time_us >= LONG_PRESS_US)
        {
            b->long_reported = true;
            if (b->long_press_callback != NULL)
            {
                b->long_press_callback();
            }
        }
        break;
    case BUTTON_STATE_RELEASING:
        if (is_low)
        {
            b->state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - b->release_time_us >= DEBOUNCE_DELAY_US)
        {
            b->state = BUTTON_STATE_RELEASED;
            // Нажатие было коротким: о долгом не сообщали, а у кнопки оно есть
            if (b->long_press_callback != NULL && !b->long_reported && b->press_callback != NULL)
            {
                b->press_callback();
            }
        }
        break;
    }
}