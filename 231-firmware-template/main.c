#include <stdio.h>
#include "pico/stdlib.h"
#include "log.h"
#include "device.h"
#include "profiling.h"
#include "stdio-text-protocol.h"
#include "api.h"
#include "led-task.h"

int main()
{
    stdio_init_all();
    profiling_init();
    stdio_text_protocol_init();
    led_task_init();

    while (1)
    {
        profiling_iteration();
        led_task_handle(); // Задача светодиода выполняется на каждой итерации суперцикла

        const command_t *command = stdio_text_protocol_handle();
        if (command != NULL)
        {
            api_handle(command);
        }
    }
}