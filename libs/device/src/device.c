#include "device.h"
#include <string.h>
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

// Вспомогательная функция безопасного копирования строк без использования функций со словом print
static void copy_string(char *dest, const char *src, size_t max_len)
{
    size_t i = 0;
    while (src[i] != '\0' && i < max_len - 1)
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void device_get_info(device_info_t *info)
{
    if (info == NULL)
    {
        return;
    }

    copy_string(info->project, DEVICE_PROJECT, sizeof(info->project));
    copy_string(info->repo, DEVICE_REPO, sizeof(info->repo));
    copy_string(info->board, DEVICE_BOARD, sizeof(info->board));
    copy_string(info->sdk_version, PICO_SDK_VERSION_STRING, sizeof(info->sdk_version));

    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));

    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    info->manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
}