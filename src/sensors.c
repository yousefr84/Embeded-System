#include "sensors.h"
#include "controller.h"
#include "app_config.h"
#include "log.h"
#include "TinyTimber.h"

#include <stdint.h>

typedef struct {
    OBJECT_HEAD;
    float base_temp;
    float noise_amp;
    uint32_t sample_id;
} TempSensor;

typedef struct {
    OBJECT_HEAD;
    float base_humidity;
    uint32_t sample_id;
} HumiditySensor;

typedef struct {
    OBJECT_HEAD;
    uint32_t event_id;
} LightSensor;

static TempSensor     temp_sensor     = { INIT_TTOBJECT_STATIC, 24.0f, TEMP_NOISE_NORMAL, 0 };
static HumiditySensor humidity_sensor = { INIT_TTOBJECT_STATIC, 50.0f, 0 };
static LightSensor    light_sensor    = { INIT_TTOBJECT_STATIC, 0 };

static uint32_t prng_state = 0xA5A5A5A5U;
static int heavy_load_extra;

static void prng_seed(uint32_t seed)
{
    prng_state = seed ? seed : 1U;
}

static uint32_t prng_next(void)
{
    prng_state = prng_state * 1664525U + 1013904223U;
    return prng_state;
}

static float prng_uniform01(void)
{
    return (float)(prng_next() & 0xFFFFFFU) / (float)0x1000000U;
}

static float prng_range(float lo, float hi)
{
    return lo + (hi - lo) * prng_uniform01();
}

static int prng_range_int(int lo, int hi)
{
    return lo + (int)(prng_next() % (uint32_t)(hi - lo + 1));
}

static void heavy_spin(void)
{
    volatile uint32_t x = 0;
    size_t i;
    if (!heavy_load_extra)
        return;
    for (i = 0; i < 120000U; i++)
        x += (uint32_t)(i ^ (i >> 2));
    (void)x;
}

static env_result_t TempSensor_sample(tt_object_t *self, void *arg)
{
    TempSensor *s = (TempSensor *)self;
    float noise, value;
    (void)arg;

    heavy_spin();
    noise = prng_range(-s->noise_amp, s->noise_amp);
    value = s->base_temp + noise;
    s->sample_id++;

    log_event(LOG_INFO, "TempSensor",
              "sample #%u: %.2f C (base=%.2f noise=%+.2f)",
              (unsigned)s->sample_id, value, s->base_temp, noise);

    controller_notify_temp(value);
    PeriodicTimer(TEMP_PERIOD_MS, s, TempSensor_sample);
    return 0;
}

static env_result_t HumiditySensor_sample(tt_object_t *self, void *arg)
{
    HumiditySensor *s = (HumiditySensor *)self;
    float drift, value;
    (void)arg;

    heavy_spin();
    drift = prng_range(-1.5f, 1.5f);
    value = s->base_humidity + drift;
    if (value < 5.0f)  value = 5.0f;
    if (value > 95.0f) value = 95.0f;
    s->sample_id++;

    log_event(LOG_INFO, "HumiditySensor",
              "sample #%u: %.1f %%", (unsigned)s->sample_id, value);

    controller_notify_humidity(value);
    PeriodicTimer(HUMIDITY_PERIOD_MS, s, HumiditySensor_sample);
    return 0;
}

static env_result_t LightSensor_event(tt_object_t *self, void *arg)
{
    LightSensor *s = (LightSensor *)self;
    int level;
    uint32_t next_ms;
    (void)arg;

    heavy_spin();
    level = prng_range_int(LIGHT_MIN_VALUE, LIGHT_MAX_VALUE);
    s->event_id++;

    log_event(LOG_INFO, "LightSensor",
              "sporadic event #%u: level=%d", (unsigned)s->event_id, level);

    controller_notify_light(level);

    next_ms = (uint32_t)prng_range_int((int)LIGHT_MIN_MS, (int)LIGHT_MAX_MS);
    AFTER(ENV_MSEC(next_ms), s, LightSensor_event, NOARG);
    return 0;
}

void sensors_set_temp_spike(int enabled)
{
    if (enabled) {
        log_event(LOG_WARN, "TempSensor", "scenario: sudden temperature increase");
        temp_sensor.base_temp = 31.0f;
    } else {
        temp_sensor.base_temp = 24.0f;
    }
}

void sensors_set_severe_noise(int enabled)
{
    temp_sensor.noise_amp = enabled ? TEMP_NOISE_SEVERE : TEMP_NOISE_NORMAL;
    if (enabled)
        log_event(LOG_WARN, "TempSensor", "scenario: severe noise (+/- %.1f C)",
                  TEMP_NOISE_SEVERE);
}

void sensors_set_heavy_load(int enabled)
{
    heavy_load_extra = enabled ? 1 : 0;
    if (enabled)
        log_event(LOG_WARN, "SYSTEM", "scenario: heavy load (extra WCET per handler)");
}

void sensors_init(void)
{
    prng_seed(42U);
    temp_sensor.noise_amp = TEMP_NOISE_NORMAL;

    log_event(LOG_INFO, "Sensors", "starting periodic and sporadic sampling");

    WITHIN(ENV_MSEC(0), ENV_MSEC(TEMP_PERIOD_MS), &temp_sensor, TempSensor_sample, NOARG);
    AFTER(ENV_MSEC(50), &humidity_sensor, HumiditySensor_sample, NOARG);
    AFTER(ENV_MSEC(200), &light_sensor, LightSensor_event, NOARG);
}
