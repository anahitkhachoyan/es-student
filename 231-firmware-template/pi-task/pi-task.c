#include "pi-task.h"
#include <stdint.h>
#include "pico/stdlib.h"
#include "systime/systime.h"

// Размер порции: один член ряда ~585 тактов (~4.7 мкс на 125 МГц).
// Порция до 0.5 мс (500 мкс) вмещает около 100 членов. Выбираем 100.
#define PI_TERMS_PER_CALL 100

static pi_state_t pi_state = PI_STATE_NOT_STARTED;
static uint32_t total_terms = 0;
static uint32_t current_k = 0;
static double partial_sum = 0.0;
static double partial_sign = 1.0;

static uint64_t start_time_us = 0;
static uint64_t duration_ms = 0;
static double final_pi = 0.0;

void pi_task_init(void)
{
    pi_state = PI_STATE_NOT_STARTED;
    total_terms = 0;
    current_k = 0;
    partial_sum = 0.0;
    partial_sign = 1.0;
    duration_ms = 0;
    final_pi = 0.0;
}

bool pi_task_start(uint32_t terms)
{
    // Если расчет уже идет — отказываем (pi is busy)
    if (pi_state == PI_STATE_RUNNING)
    {
        return false;
    }

    total_terms = terms;
    current_k = 0;
    partial_sum = 0.0;
    partial_sign = 1.0;
    pi_state = PI_STATE_RUNNING;
    start_time_us = systime_us();

    return true;
}

pi_state_t pi_task_get_state(void)
{
    return pi_state;
}

void pi_task_handle(void)
{
    if (pi_state != PI_STATE_RUNNING)
    {
        return;
    }

    uint32_t end_k = current_k + PI_TERMS_PER_CALL;
    
    // Защита от переполнения uint32_t при приближении к 4 294 967 295
    if (end_k < current_k || end_k > total_terms)
    {
        end_k = total_terms;
    }

    for (uint32_t k = current_k; k < end_k; k++)
    {
        partial_sum += partial_sign / (2.0 * k + 1.0);
        partial_sign = -partial_sign;
    }

    current_k = end_k;

    if (current_k >= total_terms)
    {
        uint64_t end_time_us = systime_us();
        duration_ms = (end_time_us - start_time_us) / 1000ULL;
        final_pi = partial_sum * 4.0;
        pi_state = PI_STATE_READY;
    }
}

bool pi_task_get_result(double *out_pi, uint32_t *out_terms, uint64_t *out_duration_ms)
{
    if (pi_state != PI_STATE_READY)
    {
        return false;
    }

    if (out_pi) *out_pi = final_pi;
    if (out_terms) *out_terms = total_terms;
    if (out_duration_ms) *out_duration_ms = duration_ms;

    return true;
}