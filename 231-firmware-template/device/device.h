#pragma once

#include <stdint.h>

// константы: имя устройства, версия прошивки, проект, ссылка на репозиторий
#define DEVICE_NAME "es-led-module"
#define FIRMWARE_VERSION "1.2.0"

#define DEVICE_PROJECT "231-firmware-template"
#define DEVICE_REPO "https://github.com/anahitkhachoyan/es-student"

// запасное значение DEVICE_BOARD на случай сборки без определения компилятора
#ifndef DEVICE_BOARD
#define DEVICE_BOARD "unknown"
#endif

struct info_t {
    uint32_t version;
    char name[13];
    uint8_t revision;
};


void device_info(void);