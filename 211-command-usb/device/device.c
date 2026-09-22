#include "device.h"

#include <stdio.h>
#include <stddef.h>
#include "pico/unique_id.h"
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

struct info_t device_card = {
    .version = 0x00010000,
    .name = "es-cmd-usb",
    .revision = 2
};

void device_info(void)
{
    char board_id[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    pico_get_unique_board_id_string(board_id, sizeof(board_id));
    
    // читаем серийный номер платы функцией SDK
    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    // читаем регистр CHIP_ID по адресу и разбираем три поля
    uint32_t manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    uint32_t part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    uint32_t revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;

    // печатаем пять строк паспорта
    printf("project: %s\n", DEVICE_PROJECT);
    printf("repo: %s\n", DEVICE_REPO);
    printf("board: %s\n", DEVICE_BOARD);
    printf("pico-sdk: %s\n", PICO_SDK_VERSION_STRING);
    printf("serial: %s\n", board_id);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n", manufacturer, part, revision);
}

void dev_info(void)
{
    unsigned fields_size = sizeof(device_card.version) + 
                           sizeof(device_card.name) + 
                           sizeof(device_card.revision);
    unsigned struct_size = sizeof(device_card);
    unsigned padding = struct_size - fields_size;

    printf("struct          address     size offset value\n");
    printf("device_card     0x%08x  %4u\n", (unsigned)(uintptr_t)&device_card, struct_size);

    printf("- version       0x%08x  %4u  %6u 0x%08x\n",
           (unsigned)(uintptr_t)&device_card.version,
           (unsigned)sizeof(device_card.version),
           (unsigned)offsetof(struct info_t, version),
           device_card.version);

    printf("- name          0x%08x  %4u  %6u %s\n",
           (unsigned)(uintptr_t)device_card.name,
           (unsigned)sizeof(device_card.name),
           (unsigned)offsetof(struct info_t, name),
           device_card.name);

    printf("- revision      0x%08x  %4u  %6u %u\n",
           (unsigned)(uintptr_t)&device_card.revision,
           (unsigned)sizeof(device_card.revision),
           (unsigned)offsetof(struct info_t, revision),
           device_card.revision);

    printf("fields %u, sizeof %u, padding %u\n", fields_size, struct_size, padding);
}