#include "sensors.h"
#include "controller.h"
#include "config.h"
#include "log.h"

#include <tT.h>
#include <env.h>
#include <stdint.h>

typedef struct {
	tt_object_t object;
	float base_temp;
	float noise_amp;
	uint32_t sample_id;
} temp_sensor_t;

typedef struct {
	tt_object_t object;
	float base_humidity;
	uint32_t sample_id;
} humidity_sensor_t;

typedef struct {
	tt_object_t object;
	uint32_t event_id;
} light_sensor_t;

static temp_sensor_t     temp_sensor     = { tt_object(), 24.0f, TEMP_NOISE_NORMAL, 0 };
static humidity_sensor_t humidity_sensor = { tt_object(), 50.0f, 0 };
static light_sensor_t    light_sensor    = { tt_object(), 0 };

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

static env_result_t temp_sensor_sample(temp_sensor_t *self, void *arg)
{
	float noise, value;
	(void)arg;

	heavy_spin();
	noise = prng_range(-self->noise_amp, self->noise_amp);
	value = self->base_temp + noise;
	self->sample_id++;

	log_event(LOG_INFO, "TempSensor",
	          "sample #%u: %.2f C (base=%.2f noise=%+.2f)",
	          (unsigned)self->sample_id, value, self->base_temp, noise);

	controller_notify_temp(value);
	TT_AFTER(ENV_MSEC(PERIOD_TEMP), self, temp_sensor_sample, TT_ARGS_NONE);
	return 0;
}

static env_result_t humidity_sensor_sample(humidity_sensor_t *self, void *arg)
{
	float drift, value;
	(void)arg;

	heavy_spin();
	drift = prng_range(-1.5f, 1.5f);
	value = self->base_humidity + drift;
	if (value < 5.0f)  value = 5.0f;
	if (value > 95.0f) value = 95.0f;
	self->sample_id++;

	log_event(LOG_INFO, "HumiditySensor",
	          "sample #%u: %.1f %%", (unsigned)self->sample_id, value);

	controller_notify_humidity(value);
	TT_AFTER(ENV_MSEC(PERIOD_HUM), self, humidity_sensor_sample, TT_ARGS_NONE);
	return 0;
}

static env_result_t light_sensor_event(light_sensor_t *self, void *arg)
{
	int level;
	uint32_t next_ms;
	(void)arg;

	heavy_spin();
	level = prng_range_int(LIGHT_MIN_VALUE, LIGHT_MAX_VALUE);
	self->event_id++;

	log_event(LOG_INFO, "LightSensor",
	          "sporadic event #%u: level=%d", (unsigned)self->event_id, level);

	controller_notify_light(level);

	next_ms = (uint32_t)prng_range_int((int)LIGHT_MIN_MS, (int)LIGHT_MAX_MS);
	TT_AFTER(ENV_MSEC(next_ms), self, light_sensor_event, TT_ARGS_NONE);
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

	TT_WITHIN(ENV_MSEC(0), ENV_MSEC(PERIOD_TEMP),
	          &temp_sensor, temp_sensor_sample, TT_ARGS_NONE);
	TT_AFTER(ENV_MSEC(50), &humidity_sensor, humidity_sensor_sample, TT_ARGS_NONE);
	TT_AFTER(ENV_MSEC(200), &light_sensor, light_sensor_event, TT_ARGS_NONE);
}
