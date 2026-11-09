#pragma once
#include <stdint.h>
#include "stdio-text-protocol/stdio-text-protocol.h"
#include "telemetry-task/telemetry-task.h"
#include "stream-task/stream-task.h"

typedef uint64_t (*api_uptime_cb_t)(void);

void api_init(api_uptime_cb_t uptime_cb);

void api_handle(const command_t *cmd);

uint64_t api_get_uptime(void);

void api_print_telemetry(const telemetry_record_t *record);

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes);