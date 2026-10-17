#include "button-task.h"
#include <stddef.h>

#define DEBOUNCE_DELAY_US 20000

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
    uint64_t transition_time_us;
    button_task_callback_t press_callback;
} button_t;

static button_t buttons[BUTTON_TASK_MAX];
static uint32_t button_count = 0;

void button_task_init(uint32_t button, button_task_callback_t on_press)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return;
    }

    buttons[button].state = BUTTON_STATE_RELEASED;
    buttons[button].press_count = 0;
    buttons[button].transition_time_us = 0;
    buttons[button].press_callback = on_press;

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

void button_task_handle(uint32_t button, bool is_low, uint64_t now_us)
{
    if (button >= BUTTON_TASK_MAX)
    {
        return;
    }

    button_t *b = &buttons[button];

    switch (b->state)
    {
    case BUTTON_STATE_RELEASED:
        if (is_low)
        {
            b->transition_time_us = now_us;
            b->state = BUTTON_STATE_PRESSING;
        }
        break;
    case BUTTON_STATE_PRESSING:
        if (!is_low)
        {
            b->state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - b->transition_time_us >= DEBOUNCE_DELAY_US)
        {
            b->press_count++;
            if (b->press_callback != NULL)
            {
                b->press_callback();
            }
            b->state = BUTTON_STATE_PRESSED;
        }
        break;
    case BUTTON_STATE_PRESSED:
        if (!is_low)
        {
            b->transition_time_us = now_us;
            b->state = BUTTON_STATE_RELEASING;
        }
        break;
    case BUTTON_STATE_RELEASING:
        if (is_low)
        {
            b->state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - b->transition_time_us >= DEBOUNCE_DELAY_US)
        {
            b->state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}