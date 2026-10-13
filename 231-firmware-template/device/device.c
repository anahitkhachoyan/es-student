#include "device.h"
#include <string.h>
#include "pico/version.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/sysinfo.h"

void device_get_info(device_info_t *info)
{
    if (info == NULL)
    {
        return;
    }

    snprintf(info->project, sizeof(info->project), "%s", DEVICE_PROJECT);
    snprintf(info->repo, sizeof(info->repo), "%s", DEVICE_REPO);
    snprintf(info->board, sizeof(info->board), "%s", DEVICE_BOARD);
    snprintf(info->sdk_version, sizeof(info->sdk_version), "%s", PICO_SDK_VERSION_STRING);

    pico_get_unique_board_id_string(info->serial, sizeof(info->serial));

    volatile uint32_t *chip_id = (uint32_t *)(SYSINFO_BASE + SYSINFO_CHIP_ID_OFFSET);
    uint32_t id = *chip_id;

    info->manufacturer = (id & SYSINFO_CHIP_ID_MANUFACTURER_BITS) >> SYSINFO_CHIP_ID_MANUFACTURER_LSB;
    info->part = (id & SYSINFO_CHIP_ID_PART_BITS) >> SYSINFO_CHIP_ID_PART_LSB;
    info->revision = (id & SYSINFO_CHIP_ID_REVISION_BITS) >> SYSINFO_CHIP_ID_REVISION_LSB;
}