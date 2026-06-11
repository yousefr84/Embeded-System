#include <tT.h>
#include <env.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Greenhouse monitoring and control using TinyTimber reactive objects. */

#define PERIOD_TEMP       200
#define PERIOD_HUM        500
#define PERIOD_CONTROLLER 100
#define LIGHT_MIN_MS      1000
#define LIGHT_MAX_MS      4000
#define TEMP_FAN_ON       28.0f
#define HUM_PUMP_ON       35.0f
#define LIGHT_THRESHOLD   40
#define TEMP_NOISE_NORMAL 0.5f
#define TEMP_NOISE_HEAVY  2.0f
#define SCENARIO_RUN_SEC  12

typedef enum {
	SCENARIO_NORMAL = 0,
	SCENARIO_TEMP_SPIKE,
	SCENARIO_SEVERE_NOISE,
	SCENARIO_HEAVY_LOAD,
	SCENARIO_FAN_FAILURE,
	SCENARIO_ALL
} scenario_id_t;

typedef struct {
	tt_object_t object;
	float temperature;
	float humidity;
	int light;
	int fan_cmd;
	int pump_cmd;
	int lamp_cmd;
	unsigned tick_count;
} controller_t;

/* Each sensor and actuator is a separate TinyTimber object. */

typedef struct {
	tt_object_t object;
	float base_temp;
	float noise_amp;
	unsigned sample_count;
} temp_sensor_t;

typedef struct {
	tt_object_t object;
	float humidity;
	unsigned sample_count;
} hum_sensor_t;

typedef struct {
	tt_object_t object;
	unsigned event_count;
} light_sensor_t;

typedef struct {
	tt_object_t object;
	int state;
	int failed;
} fan_t;

typedef struct {
	tt_object_t object;
	int state;
} pump_t;

typedef struct {
	tt_object_t object;
	int state;
} lamp_t;

typedef struct {
	tt_object_t object;
} scenario_t;

typedef struct {
	int on;
} cmd_t;

static controller_t  controller;
static temp_sensor_t temp_sensor;
static hum_sensor_t  hum_sensor;
static light_sensor_t light_sensor;
static fan_t         fan;
static pump_t        pump;
static lamp_t        lamp;
static scenario_t    scenario;

static int heavy_load;
static int g_argc;
static char **g_argv;
static struct timespec log_start;
static unsigned info_count;
static unsigned warn_count;
static unsigned miss_count;

static unsigned ms_now(void)
{
	struct timespec now;
	unsigned a;
	unsigned b;

	clock_gettime(CLOCK_MONOTONIC, &now);
	a = (unsigned)(now.tv_sec * 1000 + now.tv_nsec / 1000000);
	b = (unsigned)(log_start.tv_sec * 1000 + log_start.tv_nsec / 1000000);
	return a - b;
}

static void log_print(const char *tag, const char *comp, const char *text)
{
	printf("[%7u ms][%-5s][%-10s] %s\n", ms_now(), tag, comp, text);
	fflush(stdout);
}

static void log_info(const char *comp, const char *text)
{
	info_count++;
	log_print("INFO", comp, text);
}

static void log_warn(const char *comp, const char *text)
{
	warn_count++;
	log_print("WARN", comp, text);
}

static void log_summary(void)
{
	char buf[80];
	sprintf(buf, "info=%u warn=%u deadline_miss=%u",
	        info_count, warn_count, miss_count);
	log_print("INFO", "SUMMARY", buf);
}

static float temp_noise(void)
{
	int r;

	r = rand() % 101;
	return ((float)r / 100.0f) - 0.5f;
}

static float hum_value(void)
{
	float v;

	v = 20.0f + (float)(rand() % 61);
	return v;
}

static int light_value(void)
{
	return rand() % 101;
}

static int light_delay_ms(void)
{
	return LIGHT_MIN_MS + (rand() % (LIGHT_MAX_MS - LIGHT_MIN_MS + 1));
}

static void heavy_spin(void)
{
	volatile unsigned x;
	unsigned i;

	if (!heavy_load)
		return;

	x = 0;
	for (i = 0; i < 120000U; i++)
		x = x + (i ^ (i >> 2));
}

static env_result_t fan_set(fan_t *self, void *arg)
{
	cmd_t *cmd;
	int want;

	cmd = (cmd_t *)arg;
	want = cmd->on;

	if (self->failed) {
		if (want)
			log_warn("Fan", "FAILURE: command ignored (requested ON, stuck OFF)");
		return 0;
	}

	if (self->state != want) {
		self->state = want;
		if (want)
			log_info("Fan", "FAN -> ON");
		else
			log_info("Fan", "FAN -> OFF");
	}
	return 0;
}

static env_result_t pump_set(pump_t *self, void *arg)
{
	cmd_t *cmd;
	int want;

	cmd = (cmd_t *)arg;
	want = cmd->on;

	if (self->state != want) {
		self->state = want;
		if (want)
			log_info("Pump", "PUMP -> ON");
		else
			log_info("Pump", "PUMP -> OFF");
	}
	return 0;
}

static env_result_t lamp_set(lamp_t *self, void *arg)
{
	cmd_t *cmd;
	int want;

	cmd = (cmd_t *)arg;
	want = cmd->on;

	if (self->state != want) {
		self->state = want;
		if (want)
			log_info("Lamp", "LAMP -> ON");
		else
			log_info("Lamp", "LAMP -> OFF");
	}
	return 0;
}

static void send_fan(int on)
{
	cmd_t cmd;

	cmd.on = on;
	TT_ASYNC(&fan, fan_set, &cmd);
}

static void send_pump(int on)
{
	cmd_t cmd;

	cmd.on = on;
	TT_ASYNC(&pump, pump_set, &cmd);
}

static void send_lamp(int on)
{
	cmd_t cmd;

	cmd.on = on;
	TT_ASYNC(&lamp, lamp_set, &cmd);
}

static env_result_t controller_temp_rx(controller_t *self, void *arg)
{
	self->temperature = *(float *)arg;
	return 0;
}

static env_result_t controller_hum_rx(controller_t *self, void *arg)
{
	self->humidity = *(float *)arg;
	return 0;
}

static env_result_t controller_light_rx(controller_t *self, void *arg)
{
	self->light = *(int *)arg;
	return 0;
}

static void notify_temp(float value)
{
	TT_ASYNC(&controller, controller_temp_rx, &value);
}

static void notify_hum(float value)
{
	TT_ASYNC(&controller, controller_hum_rx, &value);
}

static void notify_light(int value)
{
	TT_ASYNC(&controller, controller_light_rx, &value);
}

static env_result_t controller_tick(controller_t *self, void *arg)
{
	int fan_on;
	int pump_on;
	int lamp_on;
	char buf[128];

	/* controller rules from project specification */
	fan_on = (self->temperature > TEMP_FAN_ON) ? 1 : 0;
	pump_on = (self->humidity < HUM_PUMP_ON) ? 1 : 0;
	lamp_on = (self->light < LIGHT_THRESHOLD) ? 1 : 0;

	if (fan_on != self->fan_cmd) {
		self->fan_cmd = fan_on;
		if (fan_on) {
			sprintf(buf, "Controller: Temp=%.1f -> Fan ON", self->temperature);
			log_info("Controller", buf);
		} else {
			sprintf(buf, "Controller: Temp=%.1f -> Fan OFF", self->temperature);
			log_info("Controller", buf);
		}
		send_fan(fan_on);
	}

	if (pump_on != self->pump_cmd) {
		self->pump_cmd = pump_on;
		if (pump_on) {
			sprintf(buf, "Controller: Hum=%.1f -> Pump ON", self->humidity);
			log_info("Controller", buf);
		} else {
			sprintf(buf, "Controller: Hum=%.1f -> Pump OFF", self->humidity);
			log_info("Controller", buf);
		}
		send_pump(pump_on);
	}

	if (lamp_on != self->lamp_cmd) {
		self->lamp_cmd = lamp_on;
		if (lamp_on) {
			sprintf(buf, "Controller: Light=%d -> Lamp ON", self->light);
			log_info("Controller", buf);
		} else {
			sprintf(buf, "Controller: Light=%d -> Lamp OFF", self->light);
			log_info("Controller", buf);
		}
		send_lamp(lamp_on);
	}

	self->tick_count++;
	sprintf(buf, "tick #%u T=%.1f H=%.1f L=%d",
	        self->tick_count, self->temperature, self->humidity, self->light);
	log_info("Controller", buf);

	TT_AFTER(ENV_MSEC(PERIOD_CONTROLLER), self, controller_tick, TT_ARGS_NONE);
	return 0;
}

static env_result_t temp_sensor_sample(temp_sensor_t *self, void *arg)
{
	float noise;
	float value;
	char buf[96];

	heavy_spin();
	noise = temp_noise() * self->noise_amp / TEMP_NOISE_NORMAL;
	value = self->base_temp + noise;
	self->sample_count++;

	sprintf(buf, "Temperature sample #%u: %.2f C", self->sample_count, value);
	log_info("TempSensor", buf);

	notify_temp(value);

	/* call periodic task temp_sensor_sample (same style as professor example) */
	TT_AFTER(ENV_MSEC(PERIOD_TEMP), self, temp_sensor_sample, TT_ARGS_NONE);
	return 0;
}

static env_result_t hum_sensor_sample(hum_sensor_t *self, void *arg)
{
	float value;
	char buf[96];

	heavy_spin();
	value = hum_value();
	self->sample_count++;

	sprintf(buf, "Humidity sample #%u: %.1f %%", self->sample_count, value);
	log_info("HumSensor", buf);

	notify_hum(value);
	TT_AFTER(ENV_MSEC(PERIOD_HUM), self, hum_sensor_sample, TT_ARGS_NONE);
	return 0;
}

static env_result_t light_sensor_event(light_sensor_t *self, void *arg)
{
	int value;
	int delay;
	char buf[96];

	heavy_spin();
	value = light_value();
	self->event_count++;

	if (value < LIGHT_THRESHOLD)
		sprintf(buf, "Light event #%u: DARK (level=%d)", self->event_count, value);
	else
		sprintf(buf, "Light event #%u: LIGHT (level=%d)", self->event_count, value);
	log_info("LightSensor", buf);

	notify_light(value);
	delay = light_delay_ms();
	TT_AFTER(ENV_MSEC(delay), self, light_sensor_event, TT_ARGS_NONE);
	return 0;
}

static env_result_t scenario_temp_spike(scenario_t *self, void *arg)
{
	log_warn("Scenario", "Sudden temperature increase");
	temp_sensor.base_temp = 31.0f;
	return 0;
}

static env_result_t scenario_load_gen(scenario_t *self, void *arg)
{
	TT_AFTER(ENV_MSEC(50), self, scenario_load_gen, TT_ARGS_NONE);
	return 0;
}

static env_result_t scenario_end(scenario_t *self, void *arg)
{
	log_summary();
	printf("\n");
	fflush(stdout);
	exit(0);
}

static void start_sensors(void)
{
	log_info("Sensors", "starting temperature, humidity and light sensors");

	TT_AFTER(ENV_MSEC(0), &temp_sensor, temp_sensor_sample, TT_ARGS_NONE);
	TT_AFTER(ENV_MSEC(50), &hum_sensor, hum_sensor_sample, TT_ARGS_NONE);
	TT_AFTER(ENV_MSEC(200), &light_sensor, light_sensor_event, TT_ARGS_NONE);
}

static void start_controller(void)
{
	char buf[64];

	sprintf(buf, "controller period %d ms", PERIOD_CONTROLLER);
	log_info("Controller", buf);

	TT_AFTER(ENV_MSEC(0), &controller, controller_tick, TT_ARGS_NONE);
}

static void schedule_stop(void)
{
	TT_AFTER(ENV_SEC(SCENARIO_RUN_SEC), &scenario, scenario_end, TT_ARGS_NONE);
}

static void setup_temp_spike(void)
{
	TT_AFTER(ENV_SEC(3), &scenario, scenario_temp_spike, TT_ARGS_NONE);
}

static void setup_severe_noise(void)
{
	temp_sensor.noise_amp = TEMP_NOISE_HEAVY;
	log_warn("Scenario", "Heavy temperature sensor noise");
}

static void setup_heavy_load(void)
{
	heavy_load = 1;
	log_warn("Scenario", "Heavy sensor load enabled");
	TT_AFTER(ENV_MSEC(100), &scenario, scenario_load_gen, TT_ARGS_NONE);
}

static void setup_fan_failure(void)
{
	fan.failed = 1;
	temp_sensor.base_temp = 31.0f;
	log_warn("Scenario", "Fan failure injected");
	log_warn("Scenario", "Sudden temperature increase");
}

static scenario_id_t parse_scenario(const char *name)
{
	if (!name || strcmp(name, "normal") == 0)
		return SCENARIO_NORMAL;
	if (strcmp(name, "temp_spike") == 0)
		return SCENARIO_TEMP_SPIKE;
	if (strcmp(name, "severe_noise") == 0)
		return SCENARIO_SEVERE_NOISE;
	if (strcmp(name, "heavy_load") == 0)
		return SCENARIO_HEAVY_LOAD;
	if (strcmp(name, "fan_failure") == 0)
		return SCENARIO_FAN_FAILURE;
	if (strcmp(name, "all") == 0)
		return SCENARIO_ALL;
	return SCENARIO_NORMAL;
}

static int run_one_scenario(const char *name)
{
	char title[80];

	info_count = 0;
	warn_count = 0;
	miss_count = 0;
	heavy_load = 0;
	clock_gettime(CLOCK_MONOTONIC, &log_start);

	controller = (controller_t){ tt_object(), 28.0f, 50.0f, 60, 0, 0, 0, 0 };
	temp_sensor = (temp_sensor_t){ tt_object(), 28.0f, TEMP_NOISE_NORMAL, 0 };
	hum_sensor = (hum_sensor_t){ tt_object(), 50.0f, 0 };
	light_sensor = (light_sensor_t){ tt_object(), 0 };
	fan = (fan_t){ tt_object(), 0, 0 };
	pump = (pump_t){ tt_object(), 0 };
	lamp = (lamp_t){ tt_object(), 0 };
	scenario = (scenario_t){ tt_object() };

	srand(42);

	sprintf(title, "Greenhouse scenario: %s", name);
	log_info("SYSTEM", title);

	start_controller();

	if (strcmp(name, "temp_spike") == 0)
		setup_temp_spike();
	else if (strcmp(name, "severe_noise") == 0)
		setup_severe_noise();
	else if (strcmp(name, "heavy_load") == 0)
		setup_heavy_load();
	else if (strcmp(name, "fan_failure") == 0)
		setup_fan_failure();

	start_sensors();
	schedule_stop();
	return 0;
}

static int run_subprocess(const char *scenario)
{
	char cmd[160];
	int n;

	n = sprintf(cmd, "%s %s", g_argv[0], scenario);
	if (n <= 0 || n >= (int)sizeof(cmd))
		return -1;
	return system(cmd);
}

static void run_scenario(scenario_id_t id)
{
	const char *name;

	if (id == SCENARIO_ALL) {
		run_subprocess("normal");
		run_subprocess("temp_spike");
		run_subprocess("severe_noise");
		run_subprocess("heavy_load");
		run_subprocess("fan_failure");
		return;
	}

	if (id == SCENARIO_TEMP_SPIKE)
		name = "temp_spike";
	else if (id == SCENARIO_SEVERE_NOISE)
		name = "severe_noise";
	else if (id == SCENARIO_HEAVY_LOAD)
		name = "heavy_load";
	else if (id == SCENARIO_FAN_FAILURE)
		name = "fan_failure";
	else
		name = "normal";

	run_one_scenario(name);
}

void init(void)
{
	scenario_id_t id;
	const char *arg;

	arg = "normal";
	if (g_argc > 1)
		arg = g_argv[1];

	id = parse_scenario(arg);
	if (g_argc > 1 && id == SCENARIO_NORMAL
	    && strcmp(arg, "normal") != 0 && strcmp(arg, "all") != 0) {
		fprintf(stderr, "Unknown scenario: %s\n", arg);
		fprintf(stderr, "Use: normal temp_spike severe_noise heavy_load fan_failure all\n");
		exit(1);
	}

	run_scenario(id);
}

int main(int argc, char **argv)
{
	/* same startup order as ENV_STARTUP(init), with argc/argv for test scenarios */
	g_argc = argc;
	g_argv = argv;
	tt_init();
	init();
	tt_run();
	return 0;
}
