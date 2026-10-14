#include "button-task.h"
#include "systime/systime.h"
#include "platform/platform.h"

// Порог подавления дребезга: ровно 20 мс в микросекундах
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

void button_task_init(button_task_callback_t on_press)
{
    press_callback = on_press;
    button_state = BUTTON_STATE_RELEASED;
    press_count = 0;
    transition_time_us = systime_us();
}

bool button_task_is_pressed(void)
{
    return (button_state == BUTTON_STATE_PRESSED || button_state == BUTTON_STATE_RELEASING);
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(void)
{
    bool is_low = platform_button_read();
    uint64_t now_us = systime_us();

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