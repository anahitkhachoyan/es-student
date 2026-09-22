#include "memory.h"
#include "command.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/regs/addressmap.h"
#include "device.h"
#include "led.h"

extern char __flash_binary_start;
extern char __flash_binary_end;
extern char __boot2_start__;
extern char __boot2_end__;
extern char __etext;
extern char __data_start__;
extern char __data_end__;
extern char __bss_start__;
extern char __bss_end__;
extern char __HeapLimit;
extern char __StackBottom;
extern char __StackTop;

int main(void);

uint32_t data_variable = 100;
uint32_t bss_variable;

static void row(const char *name, uintptr_t start, uintptr_t end)
{
    printf("%-10s 0x%08x 0x%08x %8u\n",
           name, (unsigned)start, (unsigned)end, (unsigned)(end - start));
}

void mem_info(void)
{
    printf("area       start      end        size\n");

    uintptr_t flash_start = XIP_BASE;
    uintptr_t flash_end = XIP_BASE + PICO_FLASH_SIZE_BYTES;
    row("flash", flash_start, flash_end);

    uintptr_t sram_start = SRAM_BASE;
    uintptr_t sram_end = SRAM_BASE + 270336; 
    row("sram", sram_start, sram_end);

    uintptr_t rom_start = ROM_BASE;
    uintptr_t rom_end = ROM_BASE + 16384; 
    row("rom", rom_start, rom_end);

    uintptr_t img_start = (uintptr_t)&__flash_binary_start;
    uintptr_t img_end = (uintptr_t)&__flash_binary_end;
    row("image", img_start, img_end);
    row("free", img_end, flash_end);

    uintptr_t b2_start = (uintptr_t)&__boot2_start__;
    uintptr_t b2_end = (uintptr_t)&__boot2_end__;
    row("boot2", b2_start, b2_end);

    uintptr_t text_start = (uintptr_t)&__boot2_end__;
    uintptr_t text_end = (uintptr_t)&__etext;
    row("text", text_start, text_end);

    uintptr_t data_flash_start = (uintptr_t)&__etext;
    uintptr_t data_size = (uintptr_t)&__data_end__ - (uintptr_t)&__data_start__;
    uintptr_t data_flash_end = data_flash_start + data_size;
    row("data flash", data_flash_start, data_flash_end);

    uintptr_t data_ram_start = (uintptr_t)&__data_start__;
    uintptr_t data_ram_end = (uintptr_t)&__data_end__;
    row("data ram", data_ram_start, data_ram_end);

    uintptr_t bss_start = (uintptr_t)&__bss_start__;
    uintptr_t bss_end = (uintptr_t)&__bss_end__;
    row("bss", bss_start, bss_end);

    uintptr_t heap_start = (uintptr_t)&__bss_end__;
    uintptr_t heap_end = (uintptr_t)&__HeapLimit;
    row("heap", heap_start, heap_end);

    uintptr_t stack_start = (uintptr_t)&__StackBottom;
    uintptr_t stack_end = (uintptr_t)&__StackTop;
    row("stack", stack_start, stack_end);


    unsigned b2_sz = (unsigned)(b2_end - b2_start);
    unsigned text_sz = (unsigned)(text_end - text_start);
    unsigned img_sz = (unsigned)(img_end - img_start);
    unsigned free_flash_sz = (unsigned)(flash_end - img_end);
    unsigned data_sz_u = (unsigned)data_size;
    unsigned bss_sz = (unsigned)(bss_end - bss_start);
    unsigned ram_used = data_sz_u + bss_sz;
    unsigned heap_sz = (unsigned)(heap_end - heap_start);
    unsigned stack_sz = (unsigned)(stack_end - stack_start);

    printf("\ntotal\n");
    printf("  flash image    %u = boot2 %u + text %u + data %u\n", img_sz, b2_sz, text_sz, data_sz_u);
    printf("  flash free   %u of %u\n", free_flash_sz, (unsigned)PICO_FLASH_SIZE_BYTES);
    printf("  ram used        %u = data %u + bss %u\n", ram_used, data_sz_u, bss_sz);
    printf("  ram free      %u for heap and %u for stack\n", heap_sz, stack_sz);
}

void fw_info(void)
{
    data_variable++;
    bss_variable++;

    uint16_t *main_code = (uint16_t *)((uintptr_t)main & ~1u);
    uint16_t *fw_code = (uint16_t *)((uintptr_t)fw_info & ~1u);

    uint32_t stack_variable = 1946;
    uint32_t *heap_variable = malloc(sizeof(uint32_t));

    if (heap_variable != NULL)
    {
        *heap_variable = 1951;
    }

    printf("object          address     value\n");
    printf("main            0x%08x  0x%04x\n", (unsigned)(uintptr_t)main, *main_code);
    printf("fw_info         0x%08x  0x%04x\n", (unsigned)(uintptr_t)fw_info, *fw_code);
    printf("commands        0x%08x\n", (unsigned)(uintptr_t)commands);

    for (uint i = 0; i < command_count; i++)
    {
        printf("- %-11s 0x%08x\n", commands[i].name, (unsigned)(uintptr_t)commands[i].handler);
    }

    printf("DEVICE_PROJECT  0x%08x  %s\n", (unsigned)(uintptr_t)DEVICE_PROJECT, DEVICE_PROJECT);
    printf("DEVICE_BOARD    0x%08x  %s\n", (unsigned)(uintptr_t)DEVICE_BOARD, DEVICE_BOARD);
    printf("data_variable   0x%08x  %u\n", (unsigned)(uintptr_t)&data_variable, (unsigned)data_variable);
    printf("bss_variable    0x%08x  %u\n", (unsigned)(uintptr_t)&bss_variable, (unsigned)bss_variable);
    printf("stack_variable  0x%08x  %u\n", (unsigned)(uintptr_t)&stack_variable, (unsigned)stack_variable);
    
    if (heap_variable != NULL)
    {
        printf("heap_variable   0x%08x  %u\n", (unsigned)(uintptr_t)heap_variable, *heap_variable);
    }

    free(heap_variable);
}

#define VECTOR_TABLE 0x10000100
#define SIO_GPIO_IN  0xd0000004

void boot_info(void)
{
    // Чтение таблицы векторов по фиксированному адресу
    const uint32_t *vectors = (const uint32_t *)VECTOR_TABLE;
    uint32_t stack_top = vectors[0];
    uint32_t reset_handler = vectors[1];
    uint32_t reset_even = reset_handler & ~1u;

    // Чтение регистра GPIO_IN через указатель с volatile
    volatile uint32_t *gpio_in = (volatile uint32_t *)SIO_GPIO_IN;
    unsigned int pin = led_pin();
    uint32_t led_bit = (*gpio_in >> pin) & 1u;
    bool current_gpio_get = gpio_get(pin);

    // Вывод результатов инспекции загрузки
    printf("vector table   0x%08x\n", VECTOR_TABLE);
    printf("  stack top    0x%08x\n", stack_top);
    printf("  reset        0x%08x\n", reset_handler);
    printf("  reset (even) 0x%08x\n", reset_even);
    printf("gpio in        0x%08x\n", SIO_GPIO_IN);
    printf("  led bit      %u\n", (unsigned)led_bit);
    printf("  gpio_get     %u\n", (unsigned)current_gpio_get);
}