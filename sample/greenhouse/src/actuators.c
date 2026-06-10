#include "actuators.h"
#include "log.h"

#include <tT.h>
#include <env.h>

actuator_t fan        = { tt_object(), "Fan",       ACTUATOR_OFF, 0 };
actuator_t water_pump = { tt_object(), "WaterPump", ACTUATOR_OFF, 0 };
actuator_t lamp       = { tt_object(), "Lamp",      ACTUATOR_OFF, 0 };

static env_result_t actuator_set(actuator_t *self, actuator_cmd_t *cmd)
{
	actuator_state_t desired = cmd->on ? ACTUATOR_ON : ACTUATOR_OFF;

	if (self->failed) {
		log_event(LOG_WARN, self->name,
		          "FAILURE: command ignored (requested %s, stuck at %s)",
		          desired ? "ON" : "OFF", self->state ? "ON" : "OFF");
		return 0;
	}

	if (self->state != desired) {
		log_event(LOG_INFO, self->name, "state %s -> %s",
		          self->state ? "ON" : "OFF", desired ? "ON" : "OFF");
		self->state = desired;
	}
	return 0;
}

static env_result_t fan_set(actuator_t *self, void *arg)
{
	return actuator_set(self, (actuator_cmd_t *)arg);
}

static env_result_t pump_set(actuator_t *self, void *arg)
{
	return actuator_set(self, (actuator_cmd_t *)arg);
}

static env_result_t lamp_set(actuator_t *self, void *arg)
{
	return actuator_set(self, (actuator_cmd_t *)arg);
}

void actuators_init(void)
{
}

void actuators_command_async(actuator_t *a, int on)
{
	actuator_cmd_t cmd = { .on = on };
	if (a == &fan)
		TT_ASYNC(&fan, fan_set, &cmd);
	else if (a == &water_pump)
		TT_ASYNC(&water_pump, pump_set, &cmd);
	else
		TT_ASYNC(&lamp, lamp_set, &cmd);
}

void actuators_set_fan_failed(int failed)
{
	fan.failed = failed ? 1 : 0;
	if (failed)
		log_event(LOG_WARN, "Fan", "actuator failure injected (stuck)");
}
