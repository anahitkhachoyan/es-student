#include "button-task.h"
#include <stddef.h>

#define DEBOUNCE_DELAY_US 20000

typedef enum {
    BUTTON_STATE_RELEASED,
    BUTTON_STATE_PRESSING,
    BUTTON_STATE_PRESSED,
    BUTTON_STATE_RELEASING,
} button_state_t;

static button_state_t button_state = BUTTON_STATE_RELEASED;
static uint32_t press_count = 0;
static uint64_t transition_time_us = 0;
static button_task_callback_t press_callback = NULL;
static button_task_read_t button_read = NULL;

void button_task_init(button_task_read_t read_cb, button_task_callback_t on_press)
{
    button_read = read_cb;
    press_callback = on_press;
    button_state = BUTTON_STATE_RELEASED;
    press_count = 0;
    transition_time_us = 0;
}

bool button_task_is_pressed(void)
{
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(bool is_low, uint64_t now_us)
{
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
            button_state = BUTTON_STATE_RELEASED;
        }
        else if (now_us - transition_time_us >= DEBOUNCE_DELAY_US)
        {
            press_count++;
            if (press_callback != NULL)
            {
                press_callback();
            }
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
            button_state = BUTTON_STATE_PRESSED;
        }
        else if (now_us - transition_time_us >= DEBOUNCE_DELAY_US)
        {
            button_state = BUTTON_STATE_RELEASED;
        }
        break;
    }
}