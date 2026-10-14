#include "button-task.h"
#include <stddef.h>

static bool is_pressed_flag = false;
static uint32_t press_count = 0;
static button_task_callback_t press_callback = NULL;
static button_task_read_t button_read = NULL;
static bool last_raw_state = false;

void button_task_init(button_task_read_t read_cb, button_task_callback_t on_press)
{
    button_read = read_cb;
    press_callback = on_press;
    is_pressed_flag = false;
    press_count = 0;
    last_raw_state = false;
}

bool button_task_is_pressed(void)
{
    return is_pressed_flag;
}

uint32_t button_task_get_press_count(void)
{
    return press_count;
}

void button_task_handle(void)
{
    if (button_read == NULL)
    {
        return;
    }

    bool current_raw = button_read();
    
    // Детекция фронта нажатия (переход из false в true)
    if (current_raw && !last_raw_state)
    {
        is_pressed_flag = true;
        press_count++;
        if (press_callback != NULL)
        {
            press_callback();
        }
    }
    else if (!current_raw && last_raw_state)
    {
        is_pressed_flag = false;
    }

    last_raw_state = current_raw;
}