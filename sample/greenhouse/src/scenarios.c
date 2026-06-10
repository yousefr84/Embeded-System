#include "scenarios.h"
#include "argv.h"
#include "sensors.h"
#include "actuators.h"
#include "controller.h"
#include "config.h"
#include "log.h"

#include <tT.h>
#include <env.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	tt_object_t object;
} scenario_mgr_t;

static scenario_mgr_t scenario_mgr = { tt_object() };

static env_result_t scenario_inject_temp_spike(scenario_mgr_t *self, void *arg)
{
	(void)self;
	(void)arg;
	sensors_set_temp_spike(1);
	return 0;
}

static env_result_t scenario_load_gen(scenario_mgr_t *self, void *arg)
{
	(void)arg;
	TT_AFTER(ENV_MSEC(50), self, scenario_load_gen, TT_ARGS_NONE);
	return 0;
}

static env_result_t scenario_end(scenario_mgr_t *self, void *arg)
{
	(void)self;
	(void)arg;
	log_stats_print();
	printf("\n");
	fflush(stdout);
	exit(0);
}

static void schedule_scenario_stop(void)
{
	TT_AFTER(ENV_SEC(SCENARIO_RUN_SEC), &scenario_mgr, scenario_end, TT_ARGS_NONE);
}

static void setup_application(const char *name, void (*setup)(void))
{
	log_init(name);
	actuators_init();
	controller_init();
	if (setup)
		setup();
	sensors_init();
	schedule_scenario_stop();
}

static void setup_normal(void) { }

static void setup_temp_spike(void)
{
	TT_AFTER(ENV_SEC(3), &scenario_mgr, scenario_inject_temp_spike, TT_ARGS_NONE);
}

static void setup_severe_noise(void)
{
	sensors_set_severe_noise(1);
}

static void setup_heavy_load(void)
{
	sensors_set_heavy_load(1);
	TT_WITHIN(ENV_MSEC(100), ENV_MSEC(50),
	          &scenario_mgr, scenario_load_gen, TT_ARGS_NONE);
}

static void setup_fan_failure(void)
{
	actuators_set_fan_failed(1);
	sensors_set_temp_spike(1);
}

const char *scenario_name(scenario_id_t id)
{
	switch (id) {
	case SCENARIO_TEMP_SPIKE:   return "temp_spike";
	case SCENARIO_SEVERE_NOISE: return "severe_noise";
	case SCENARIO_HEAVY_LOAD:   return "heavy_load";
	case SCENARIO_FAN_FAILURE:  return "fan_failure";
	case SCENARIO_ALL:          return "all";
	default:                    return "normal";
	}
}

scenario_id_t scenario_parse(const char *name)
{
	if (!name || strcmp(name, "normal") == 0)
		return SCENARIO_NORMAL;
	if (strcmp(name, "temp_spike") == 0)    return SCENARIO_TEMP_SPIKE;
	if (strcmp(name, "severe_noise") == 0)  return SCENARIO_SEVERE_NOISE;
	if (strcmp(name, "heavy_load") == 0)    return SCENARIO_HEAVY_LOAD;
	if (strcmp(name, "fan_failure") == 0)   return SCENARIO_FAN_FAILURE;
	if (strcmp(name, "all") == 0)           return SCENARIO_ALL;
	return SCENARIO_NORMAL;
}

static int run_subprocess(const char *scenario)
{
	char cmd[128];
	int n = snprintf(cmd, sizeof(cmd), "%s %s", greenhouse_get_argv()[0], scenario);
	if (n < 0 || (size_t)n >= sizeof(cmd))
		return -1;
	return system(cmd);
}

void scenario_run(scenario_id_t id)
{
	if (id == SCENARIO_ALL) {
		run_subprocess("normal");
		run_subprocess("temp_spike");
		run_subprocess("severe_noise");
		run_subprocess("heavy_load");
		run_subprocess("fan_failure");
		return;
	}

	switch (id) {
	case SCENARIO_TEMP_SPIKE:
		setup_application("temp_spike", setup_temp_spike);
		break;
	case SCENARIO_SEVERE_NOISE:
		setup_application("severe_noise", setup_severe_noise);
		break;
	case SCENARIO_HEAVY_LOAD:
		setup_application("heavy_load", setup_heavy_load);
		break;
	case SCENARIO_FAN_FAILURE:
		setup_application("fan_failure", setup_fan_failure);
		break;
	default:
		setup_application("normal", setup_normal);
		break;
	}
}

void init(void)
{
	scenario_id_t id = SCENARIO_ALL;
	int argc = greenhouse_get_argc();
	char **argv = greenhouse_get_argv();
	const char *arg = "all";

	if (argc > 1)
		arg = argv[1];

	id = scenario_parse(arg);
	if (argc > 1 && id == SCENARIO_NORMAL && strcmp(arg, "normal") != 0
	    && strcmp(arg, "all") != 0) {
		fprintf(stderr, "Unknown scenario '%s'\n", arg);
		fprintf(stderr, "Use: normal, temp_spike, severe_noise, heavy_load, fan_failure, all\n");
		exit(1);
	}

	printf("Greenhouse — official TinyTimber (env/posix)\n");
	scenario_run(id);
}
