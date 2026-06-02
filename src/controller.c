#include "controller.h"
#include "actuators.h"
#include "app_config.h"
#include "log.h"

Controller greenhouse_controller = {
    INIT_TTOBJECT_STATIC,
    22.0f, 55.0f, 60, 0, 0, 0, 0
};

static void apply_control_rules(Controller *c)
{
    int fan_on  = (c->temperature_c > TEMP_FAN_ON_C) ? 1 : 0;
    int pump_on = (c->humidity_pct < HUMIDITY_PUMP_ON_PCT) ? 1 : 0;
    int lamp_on = (c->light_level < LIGHT_LAMP_ON_LEVEL) ? 1 : 0;

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

static env_result_t Controller_tick(tt_object_t *self, void *arg)
{
    Controller *c = (Controller *)self;
    (void)arg;

    c->tick_count++;
    apply_control_rules(c);

    log_event(LOG_INFO, "Controller",
              "tick #%u | T=%.2f H=%.1f L=%d | cmds fan=%d pump=%d lamp=%d",
              (unsigned)c->tick_count, c->temperature_c, c->humidity_pct,
              c->light_level, c->fan_cmd, c->pump_cmd, c->lamp_cmd);

    PeriodicTimer(CONTROLLER_PERIOD_MS, c, Controller_tick);
    return 0;
}

static env_result_t Controller_tempRx(tt_object_t *self, void *arg)
{
    Controller *c = (Controller *)self;
    c->temperature_c = *(float *)arg;
    return 0;
}

static env_result_t Controller_humRx(tt_object_t *self, void *arg)
{
    Controller *c = (Controller *)self;
    c->humidity_pct = *(float *)arg;
    return 0;
}

static env_result_t Controller_lightRx(tt_object_t *self, void *arg)
{
    Controller *c = (Controller *)self;
    c->light_level = *(int *)arg;
    return 0;
}

void controller_notify_temp(float value)
{
    ASYNC(&greenhouse_controller, Controller_tempRx, &value);
}

void controller_notify_humidity(float value)
{
    ASYNC(&greenhouse_controller, Controller_humRx, &value);
}

void controller_notify_light(int value)
{
    ASYNC(&greenhouse_controller, Controller_lightRx, &value);
}

void controller_init(void)
{
    Controller *c = &greenhouse_controller;

    log_event(LOG_INFO, "Controller",
              "initialized (period %u ms, deadline %u ms)",
              (unsigned)CONTROLLER_PERIOD_MS, (unsigned)CONTROLLER_DEADLINE_MS);

    WITHIN(ENV_MSEC(0), ENV_MSEC(CONTROLLER_DEADLINE_MS), c, Controller_tick, NOARG);
}
