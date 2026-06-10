#include "controller.h"
#include "actuators.h"
#include "config.h"
#include "log.h"

#include <tT.h>
#include <env.h>

controller_t greenhouse_controller = {
	tt_object(), 22.0f, 55.0f, 60, 0, 0, 0, 0
};

static void apply_control_rules(controller_t *c)
{
	int fan_on  = (c->temperature_c > TEMP_FAN_ON) ? 1 : 0;
	int pump_on = (c->humidity_pct < HUM_PUMP_ON) ? 1 : 0;
	int lamp_on = (c->light_level < LIGHT_LAMP_ON) ? 1 : 0;

	if (fan_on != c->fan_cmd) {
		log_event(LOG_INFO, "Controller", "fan command -> %s (T=%.2f C)",
		          fan_on ? "ON" : "OFF", c->temperature_c);
		c->fan_cmd = fan_on;
		actuators_command_async(&fan, fan_on);
	}
	if (pump_on != c->pump_cmd) {
		log_event(LOG_INFO, "Controller", "pump command -> %s (H=%.1f %%)",
		          pump_on ? "ON" : "OFF", c->humidity_pct);
		c->pump_cmd = pump_on;
		actuators_command_async(&water_pump, pump_on);
	}
	if (lamp_on != c->lamp_cmd) {
		log_event(LOG_INFO, "Controller", "lamp command -> %s (L=%d)",
		          lamp_on ? "ON" : "OFF", c->light_level);
		c->lamp_cmd = lamp_on;
		actuators_command_async(&lamp, lamp_on);
	}
}

static env_result_t controller_tick(controller_t *self, void *arg)
{
	(void)arg;

	self->tick_count++;
	apply_control_rules(self);

	log_event(LOG_INFO, "Controller",
	          "tick #%u | T=%.2f H=%.1f L=%d | cmds fan=%d pump=%d lamp=%d",
	          (unsigned)self->tick_count, self->temperature_c, self->humidity_pct,
	          self->light_level, self->fan_cmd, self->pump_cmd, self->lamp_cmd);

	TT_AFTER(ENV_MSEC(PERIOD_CONTROLLER), self, controller_tick, TT_ARGS_NONE);
	return 0;
}

static env_result_t controller_temp_rx(controller_t *self, void *arg)
{
	self->temperature_c = *(float *)arg;
	return 0;
}

static env_result_t controller_hum_rx(controller_t *self, void *arg)
{
	self->humidity_pct = *(float *)arg;
	return 0;
}

static env_result_t controller_light_rx(controller_t *self, void *arg)
{
	self->light_level = *(int *)arg;
	return 0;
}

void controller_notify_temp(float value)
{
	TT_ASYNC(&greenhouse_controller, controller_temp_rx, &value);
}

void controller_notify_humidity(float value)
{
	TT_ASYNC(&greenhouse_controller, controller_hum_rx, &value);
}

void controller_notify_light(int value)
{
	TT_ASYNC(&greenhouse_controller, controller_light_rx, &value);
}

void controller_init(void)
{
	controller_t *c = &greenhouse_controller;

	log_event(LOG_INFO, "Controller",
	          "initialized (period %u ms, deadline %u ms)",
	          (unsigned)PERIOD_CONTROLLER, (unsigned)DEADLINE_CONTROLLER);

	TT_WITHIN(ENV_MSEC(0), ENV_MSEC(DEADLINE_CONTROLLER),
	          c, controller_tick, TT_ARGS_NONE);
}
