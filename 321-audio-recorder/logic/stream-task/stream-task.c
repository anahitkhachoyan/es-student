#include "stream-task.h"
#include "platform/platform.h"
#include <stddef.h>

static stream_state_t state = STREAM_STATE_IDLE;
static stream_task_callback_t frame_callback = NULL;
static uint8_t stream_bits = 16;
static uint16_t frame_number = 0;
static uint32_t frame_total = 0;
static uint32_t sample_total = 0;

void stream_task_init(stream_task_callback_t on_frame)
{
    frame_callback = on_frame;
    state = STREAM_STATE_IDLE;
}

bool stream_task_start(uint8_t bits)
{

    if (state == STREAM_STATE_RUN || !platform_stream_start(bits))
    {
        return false;
    }

    stream_bits = bits;
    frame_number = 0;
    frame_total = 0;
    sample_total = 0;
    state = STREAM_STATE_RUN;
    return true;
}

void stream_task_stop(void)
{
    if (state == STREAM_STATE_RUN)
    {
        platform_stream_stop();
        state = STREAM_STATE_IDLE;
    }
}

stream_state_t stream_task_get_state(void)
{
    return state;
}

uint8_t stream_task_get_bits(void)
{
    return stream_bits;
}

uint32_t stream_task_get_frames(void)
{
    return frame_total;
}

uint32_t stream_task_get_samples(void)
{
    return sample_total;
}

uint32_t stream_task_get_overruns(void)
{
    return platform_stream_overruns();
}

void stream_task_handle(void)
{
    if (state != STREAM_STATE_RUN)
    {
        return;
    }

    uint32_t samples = 0;
    const uint8_t *data = platform_stream_next(&samples);
    if (data == NULL)
    {
        return;
    }

    stream_header_t header;
    header.sync_word = STREAM_SYNC_WORD;
    header.number = frame_number;
    header.count = (uint16_t)samples;

    uint32_t bytes = (stream_bits == 16) ? samples * 2 : samples;

    if (frame_callback != NULL)
    {
        frame_callback(&header, data, bytes);
    }

    platform_stream_release();

    frame_number++;
    frame_total++;
    sample_total += samples;
}