#include "api-commands.h"
#include "api.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>
#include "device/device.h"
#include "firmware.h"
#include "platform/platform.h"
#include "profiling/profiling.h"
#include "led-task.h"
#include "button-task.h"
#include "pi-task.h"
#include "block-task/block-task.h"
#include "stream-task/stream-task.h"

#define TELEMETRY_DEFAULT_PERIOD_MS 100

#define STREAM_BLINK_PERIOD_MS 250

#define MIC_DEFAULT_WINDOW_MS 100

#define MIC_BIAS_SAMPLES 4410

#define CAP_PER_LINE 16

static bool parse_u32(const char *str, uint32_t *out_value)
{
    if (str == NULL || *str == '\0')
    {
        return false;
    }

    if (*str == '-' || *str == '+')
    {
        return false;
    }

    char *endptr;
    errno = 0;
    unsigned long val = strtoul(str, &endptr, 10);

    if (errno != 0 || *endptr != '\0')
    {
        return false;
    }

    *out_value = (uint32_t)val;
    return true;
}

static void print_led_status(void)
{
    char buf[64];
    led_task_get_status_str(buf, sizeof(buf));
    printf("%s\n", buf);
}

static void cmd_info(const command_t *cmd)
{
    (void)cmd;
    device_info_t info;
    device_get_info(&info);

    printf("name: %s\n", FIRMWARE_NAME);
    printf("version: %s\n", FIRMWARE_VERSION);
    printf("project: %s\n", FIRMWARE_PROJECT);
    printf("repo: %s\n", info.repo);
    printf("board: %s\n", info.board);
    printf("serial: %s\n", info.serial);
    printf("chip: manufacturer 0x%03x, part 0x%04x, revision %u\n",
           (unsigned)info.manufacturer, (unsigned)info.part, (unsigned)info.revision);
    printf("pico-sdk: %s\n", info.sdk_version);
}

static void cmd_uptime(const command_t *cmd)
{
    (void)cmd;
    printf("uptime: %llu ms\n", (unsigned long long)api_get_uptime());
}

static void cmd_profiling(const command_t *cmd)
{
    (void)cmd;
#if PROFILING_ENABLED
    profiling_result_t res;
    for (uint32_t i = 0; i < PROFILING_MAX_STOPWATCHES; i++)
    {
        if (profiling_get(i, &res))
        {

            printf("%-10s count %-10u mean %.2f us  max %u us\n",
                   res.name,
                   (unsigned)res.count,
                   res.mean_us,
                   (unsigned)res.max_us);
        }
    }
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_profiling_reset(const command_t *cmd)
{
    (void)cmd;
#if PROFILING_ENABLED
    profiling_reset();
    printf("profiling reset\n");
#else
    printf("profiling is disabled in this build\n");
#endif
}

static void cmd_get_voltage(const command_t *cmd)
{
    (void)cmd;
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("voltage: %.3f V\n", (double)platform_voltage_read());
}

static void cmd_get_temp(const command_t *cmd)
{
    (void)cmd;
    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }
    printf("temp: %.1f C\n", (double)platform_temp_read());
}

void api_print_telemetry(const telemetry_record_t *record)
{
    printf("tm: %llu %.3f %.1f\n",
           (unsigned long long)record->time_us,
           (double)record->voltage_v,
           (double)record->temp_c);
}

static void cmd_tm_start(const command_t *cmd)
{
    uint32_t period = TELEMETRY_DEFAULT_PERIOD_MS;

    if (cmd->argc > 1)
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (cmd->argc == 1 && !parse_u32(cmd->argv[0], &period))
    {
        printf("error: usage tm_start [<period_ms>]\n");
        return;
    }

    if (platform_adc_busy())
    {
        printf("error: adc busy\n");
        return;
    }

    if (!telemetry_task_start(period))
    {
        printf("error: period_ms must be at least 10\n");
        return;
    }

    printf("telemetry: started, period %u ms\n", (unsigned)period);
}

static void cmd_tm_stop(const command_t *cmd)
{
    (void)cmd;
    telemetry_task_stop();
    printf("telemetry: stopped\n");
}

static void cmd_charge_on(const command_t *cmd)
{
    (void)cmd;
    platform_charge_set(true);
    printf("charge: on\n");
}

static void cmd_charge_off(const command_t *cmd)
{
    (void)cmd;
    platform_charge_set(false);
    printf("charge: off\n");
}

static bool telemetry_running(void)
{
    return telemetry_task_get_state() == TELEMETRY_STATE_RUN;
}

static bool start_block(uint32_t count, block_task_callback_t on_block)
{
    if (telemetry_running() || !block_task_start(count, on_block))
    {
        printf("error: adc busy\n");
        return false;
    }
    return true;
}

static void cmd_timer_blink_start(const command_t *cmd)
{
    uint32_t period = 0;
    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &period))
    {
        printf("error: usage timer_blink_start <period_ms>\n");
        return;
    }

    if (!platform_blink_start(period))
    {
        printf("error: period_ms must be 10..60000\n");
        return;
    }

    led_task_set_state(LED_STATE_OFF);
    printf("timer_blink: started, period %u ms\n", (unsigned)period);
}

static void cmd_timer_blink_stop(const command_t *cmd)
{
    (void)cmd;
    printf("timer_blink: stopped, %u toggles\n", (unsigned)platform_blink_stop());
}

static void cmd_cap_start(const command_t *cmd)
{
    uint32_t count = 0;
    uint32_t period = 0;
    if (cmd->argc != 2 || !parse_u32(cmd->argv[0], &count) || !parse_u32(cmd->argv[1], &period))
    {
        printf("error: usage cap_start <N> <period_us>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_CAPTURE_MAX || period < 10)
    {
        printf("error: N must be 1..%u, period_us at least 10\n", (unsigned)PLATFORM_CAPTURE_MAX);
        return;
    }

    if (telemetry_running() || !platform_capture_start(count, period))
    {
        printf("error: adc busy\n");
        return;
    }

    printf("capture: started, %u samples, period %u us\n", (unsigned)count, (unsigned)period);
}

static void cmd_cap_dump(const command_t *cmd)
{
    (void)cmd;
    if (!platform_capture_done())
    {
        if (platform_adc_busy())
        {
            printf("error: adc busy\n");
        }
        else
        {
            printf("error: no capture\n");
        }
        return;
    }

    platform_capture_t capture;
    platform_capture_get(&capture);

    printf("capture: %u samples, period %u us, interval %u..%u us, handler %.1f %%\n",
           (unsigned)capture.count, (unsigned)capture.period_us,
           (unsigned)capture.min_interval_us, (unsigned)capture.max_interval_us,
           (double)capture.handler_percent);

    for (uint32_t i = 0; i < capture.count; i += CAP_PER_LINE)
    {
        printf("cap:");
        for (uint32_t j = i; j < i + CAP_PER_LINE && j < capture.count; j++)
        {
            printf(" %u", (unsigned)capture.samples[j]);
        }
        printf("\n");
    }
}

static void print_dma_cap(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    uint32_t smallest = samples[0];
    uint32_t largest = samples[0];
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        if (samples[i] < smallest)
        {
            smallest = samples[i];
        }
        if (samples[i] > largest)
        {
            largest = samples[i];
        }
        sum += samples[i];
    }

    uint64_t expected_us = (uint64_t)count * 1000000ULL / PLATFORM_SAMPLE_RATE_HZ;
    printf("dma_cap: %u samples in %u us, expected %llu us, min %u max %u mean %.1f\n",
           (unsigned)count, (unsigned)time_us, (unsigned long long)expected_us,
           (unsigned)smallest, (unsigned)largest, (double)((float)sum / (float)count));
}

static void cmd_dma_cap(const command_t *cmd)
{
    uint32_t count = 0;
    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &count))
    {
        printf("error: usage dma_cap <N>\n");
        return;
    }

    if (count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        printf("error: N must be 1..%u\n", (unsigned)PLATFORM_BLOCK_MAX);
        return;
    }

    start_block(count, print_dma_cap);
}

static void block_stats(const uint16_t *samples, uint32_t count, float *mean, float *rms, float *peak)
{
    uint32_t sum = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        sum += samples[i];
    }
    int32_t center = (int32_t)(sum / count);

    // сумма и квадраты в целых числах: float на Cortex-M0+ медленнее в десятки раз
    uint64_t squares = 0;
    uint32_t largest = 0;
    for (uint32_t i = 0; i < count; i++)
    {
        int32_t deviation = (int32_t)samples[i] - center;
        uint32_t size = deviation < 0 ? (uint32_t)(-deviation) : (uint32_t)deviation;
        squares += (uint64_t)((int64_t)deviation * deviation);
        if (size > largest)
        {
            largest = size;
        }
    }

    *mean = (float)sum / (float)count;
    *rms = sqrtf((float)squares / (float)count);
    *peak = (float)largest;
}

static void print_mic_bias(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);
    printf("mic: bias %.3f V\n", (double)platform_adc_volts(mean));
}

static void print_mic_level(const uint16_t *samples, uint32_t count, uint32_t time_us)
{
    (void)time_us;
    float mean, rms, peak;
    block_stats(samples, count, &mean, &rms, &peak);

    uint64_t window_ms = ((uint64_t)count * 1000ULL + PLATFORM_SAMPLE_RATE_HZ / 2) / PLATFORM_SAMPLE_RATE_HZ;
    printf("mic: level %.1f mV rms, peak %.1f mV, %u ms\n",
           (double)(platform_adc_volts(rms) * 1000.0f),
           (double)(platform_adc_volts(peak) * 1000.0f),
           (unsigned)window_ms);
}

static void cmd_mic_bias(const command_t *cmd)
{
    (void)cmd;
    start_block(MIC_BIAS_SAMPLES, print_mic_bias);
}

static void cmd_mic_level(const command_t *cmd)
{
    uint32_t window_ms = MIC_DEFAULT_WINDOW_MS;
    if (cmd->argc > 1 || (cmd->argc == 1 && !parse_u32(cmd->argv[0], &window_ms)))
    {
        printf("error: usage mic_level [<ms>]\n");
        return;
    }

    uint64_t count = (uint64_t)window_ms * PLATFORM_SAMPLE_RATE_HZ / 1000ULL;
    if (count == 0 || count > PLATFORM_BLOCK_MAX)
    {
        printf("error: ms must be 1..185\n");
        return;
    }

    start_block((uint32_t)count, print_mic_level);
}

static void cmd_generate_tone(const command_t *cmd)
{
    uint32_t freq = 0;
    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &freq))
    {
        printf("error: usage generate_tone <Hz>\n");
        return;
    }

    if (!platform_tone_set(freq))
    {
        printf("error: Hz must be 0 or 20..20000\n");
        return;
    }

    if (freq == 0)
    {
        printf("tone: off\n");
    }
    else
    {
        printf("tone: %u Hz\n", (unsigned)freq);
    }
}

void api_send_frame(const stream_header_t *header, const uint8_t *data, uint32_t bytes)
{
    platform_write(header, sizeof(*header));
    platform_write(data, bytes);
}

static void cmd_stream_start(const command_t *cmd)
{
    uint32_t bits = 16;
    if (cmd->argc > 1 ||
        (cmd->argc == 1 && (!parse_u32(cmd->argv[0], &bits) || (bits != 8 && bits != 16))))
    {
        printf("error: usage stream_start [8|16]\n");
        return;
    }

    if (telemetry_running() || !stream_task_start((uint8_t)bits))
    {
        printf("error: adc busy\n");
        return;
    }

    platform_blink_start(STREAM_BLINK_PERIOD_MS);
    led_task_set_state(LED_STATE_OFF);
    printf("stream: started, %u Hz, %u bit\n", (unsigned)PLATFORM_SAMPLE_RATE_HZ, (unsigned)bits);
}

static void cmd_stream_stop(const command_t *cmd)
{
    (void)cmd;
    if (stream_task_get_state() == STREAM_STATE_RUN)
    {
        stream_task_stop();
        platform_blink_stop();
    }

    printf("stream: stopped, %u frames, %u samples, %u overruns\n",
           (unsigned)stream_task_get_frames(), (unsigned)stream_task_get_samples(),
           (unsigned)stream_task_get_overruns());
}

static void cmd_wdt_enable(const command_t *cmd)
{
    uint32_t timeout = 0;
    if (cmd->argc != 1 || !parse_u32(cmd->argv[0], &timeout))
    {
        printf("error: usage wdt_enable <timeout_ms>\n");
        return;
    }

    if (!platform_watchdog_enable(timeout))
    {
        printf("error: timeout_ms must be 1..8388\n");
        return;
    }

    printf("wdt: enabled, timeout %u ms\n", (unsigned)timeout);
}

static void cmd_hang(const command_t *cmd)
{
    (void)cmd;
    printf("hang: superloop stopped\n");

    while (1)
    {
    }
}

static void cmd_reset_reason(const command_t *cmd)
{
    (void)cmd;
    uint32_t restarts = 0;
    bool watchdog = platform_watchdog_caused_reset(&restarts);
    printf("reset: %s, restarts %u\n", watchdog ? "watchdog" : "other", (unsigned)restarts);
}

static void cmd_led_enable(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_ON);
    print_led_status();
}

static void cmd_led_disable(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_OFF);
    print_led_status();
}

static void cmd_led_blink(const command_t *cmd)
{
    (void)cmd;
    led_task_set_state(LED_STATE_BLINK);
    print_led_status();
}

static void cmd_led_period(const command_t *cmd)
{
    if (cmd->argc != 1)
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    uint32_t new_period = 0;
    if (!parse_u32(cmd->argv[0], &new_period))
    {
        printf("error: usage led_period <period_ms>\n");
        return;
    }

    if (new_period == 0)
    {
        printf("error: period_ms must be greater than 0\n");
        return;
    }

    led_task_set_period_ms(new_period);
    if (led_task_get_state() == LED_STATE_BLINK)
    {
        led_task_set_state(LED_STATE_BLINK);
    }
    print_led_status();
}

static void cmd_button(const command_t *cmd)
{
    (void)cmd;
    const char *status_str = button_task_is_pressed() ? "pressed" : "released";
    printf("button: %s, presses %u\n", status_str, (unsigned)button_task_get_press_count());
}

static void cmd_pi_start(const command_t *cmd)
{
    uint32_t terms = 1000000;

    if (cmd->argc > 1)
    {
        printf("error: usage pi_start [terms]\n");
        return;
    }

    if (cmd->argc == 1)
    {
        if (strcmp(cmd->argv[0], "0") == 0)
        {
            printf("error: terms must be greater than 0\n");
            return;
        }

        if (!parse_u32(cmd->argv[0], &terms) || terms == 0)
        {
            printf("error: usage pi_start [terms]\n");
            return;
        }
    }

    if (!pi_task_start(terms))
    {
        printf("error: pi is busy\n");
        return;
    }

    printf("pi: started, %u terms\n", (unsigned)terms);
}

static void cmd_pi(const command_t *cmd)
{
    (void)cmd;
    pi_state_t state = pi_task_get_state();

    if (state == PI_STATE_NOT_STARTED)
    {
        printf("pi: not started\n");
    }
    else if (state == PI_STATE_RUNNING)
    {
        printf("pi: running\n");
    }
    else if (state == PI_STATE_READY)
    {
        double pi_val = 0.0;
        uint64_t dur_ms = 0;
        pi_task_get_result(&pi_val, NULL, &dur_ms);
        printf("pi: %.9f in %llu ms\n", pi_val, (unsigned long long)dur_ms);
    }
}

const api_command_t api_commands[] = {
    { "info",            "device passport",                          cmd_info },
    { "uptime",          "time since reset",                         cmd_uptime },
    { "profiling",       "profiling results for all stopwatches",    cmd_profiling },
    { "profiling_reset", "reset profiling stopwatches",              cmd_profiling_reset },
    { "led_enable",      "turn on led",                              cmd_led_enable },
    { "led_disable",     "turn off led",                             cmd_led_disable },
    { "led_blink",       "blink led",                                cmd_led_blink },
    { "led_period",      "set blink period: led_period <period_ms>", cmd_led_period },
    { "button",          "button status and press count",            cmd_button },
    { "pi_start",        "start pi calculation: pi_start [terms]",   cmd_pi_start },
    { "pi",              "get pi calculation status or result",      cmd_pi },
    { "get_voltage",     "voltage on GP26, ADC input 0",             cmd_get_voltage },
    { "get_temp",        "chip temperature, degrees Celsius",        cmd_get_temp },
    { "tm_start",        "start telemetry: tm_start [<period_ms>]",  cmd_tm_start },
    { "tm_stop",         "stop telemetry",                           cmd_tm_stop },
    { "charge_on",       "charge RC circuit, 3.3 V on GP22",         cmd_charge_on },
    { "charge_off",      "discharge RC circuit, 0 V on GP22",        cmd_charge_off },
    { "timer_blink_start", "blink led by timer: timer_blink_start <period_ms>", cmd_timer_blink_start },
    { "timer_blink_stop", "stop timer blink, number of toggles",     cmd_timer_blink_stop },
    { "cap_start",       "capture by timer: cap_start <N> <period_us>", cmd_cap_start },
    { "cap_dump",        "print timer capture result",               cmd_cap_dump },
    { "dma_cap",         "capture block by DMA: dma_cap <N>",        cmd_dma_cap },
    { "generate_tone",   "tone on GP20: generate_tone <Hz>, 0 is off", cmd_generate_tone },
    { "mic_bias",        "microphone bias voltage",                  cmd_mic_bias },
    { "mic_level",       "microphone level: mic_level [<ms>]",       cmd_mic_level },
    { "stream_start",    "start stream: stream_start [8|16]",        cmd_stream_start },
    { "stream_stop",     "stop stream",                              cmd_stream_stop },
    { "wdt_enable",      "enable watchdog: wdt_enable <timeout_ms>", cmd_wdt_enable },
    { "hang",            "hang the superloop on purpose",            cmd_hang },
    { "reset_reason",    "reason of the last reset",                 cmd_reset_reason },
};

const uint32_t api_command_count = sizeof(api_commands) / sizeof(api_commands[0]);