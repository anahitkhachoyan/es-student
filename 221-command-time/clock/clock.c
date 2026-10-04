#include "clock.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/regs/clocks.h"
#include "log.h"

const uint32_t CLK_SYS_LOW_KHZ = 62500;

static void clk_sys_set(uint32_t khz)
{
    if (set_sys_clock_khz(khz, false))
    {
        LOG_INF("clk_sys %u kHz\n", (unsigned)khz);
    }
    else
    {
        LOG_ERR("clk_sys %u kHz is not set\n", (unsigned)khz);
    }
}

void clk_sys_low(void)
{
    clk_sys_set(CLK_SYS_LOW_KHZ);
}

void clk_sys_default(void)
{
    clk_sys_set(SYS_CLK_KHZ);
}

// Вспомогательная функция для форматированного вывода строки таблицы
static void row(const char *name, uint32_t set_khz, uint32_t measured_khz)
{
    printf("%-8s %9u %12u\n", name, (unsigned)set_khz, (unsigned)measured_khz);
}

// Специальная строка для ROSC (у него нет настроенной частоты, поэтому вместо числа — прочерк)
static void row_rosc(const char *name, uint32_t measured_khz)
{
    printf("%-8s        - %12u\n", name, (unsigned)measured_khz);
}

void clk_info(void)
{
    printf("signal    set_khz measured_khz\n");

    // 1. clk_ref (12 МГц)
    row("clk_ref", 
        clock_get_hz(clk_ref) / 1000, 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_REF));

    // 2. clk_sys (125 МГц)
    row("clk_sys", 
        clock_get_hz(clk_sys) / 1000, 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_SYS));

    // 3. clk_peri (125 МГц)
    row("clk_peri", 
        clock_get_hz(clk_peri) / 1000, 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_PERI));

    // 4. clk_usb (48 МГц)
    row("clk_usb", 
        clock_get_hz(clk_usb) / 1000, 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_USB));

    // 5. clk_adc (48 МГц)
    row("clk_adc", 
        clock_get_hz(clk_adc) / 1000, 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_CLK_ADC));

    // 6. ROSC (кольцевой генератор, настраивается не через SDK, выводим прочерк и измеренное значение)
    row_rosc("rosc", 
        frequency_count_khz(CLOCKS_FC0_SRC_VALUE_ROSC_CLKSRC));
}

void uptime(void)
{
    printf("uptime: %llu ms\n", time_us_64() / 1000);
}