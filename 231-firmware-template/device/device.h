#pragma once

#include <stdint.h>
#include <stddef.h>
#include "pico/unique_id.h"

// константы: имя устройства, версия прошивки, проект, ссылка на репозиторий
#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.2.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/anahitkhachoyan/es-student"

// запасное значение DEVICE_BOARD на случай сборки без определения компилятора
#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

typedef struct {
    char board[32];
    char project[32];
    char repo[64];
    char sdk_version[32];
    char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES * 2 + 1];
    uint32_t manufacturer;
    uint32_t part;
    uint32_t revision;
} device_info_t;

void device_get_info(device_info_t *info);