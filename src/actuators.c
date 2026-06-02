#include "actuators.h"
#include "log.h"

Actuator fan        = { INIT_TTOBJECT_STATIC, "Fan",        ACTUATOR_OFF, 0 };
Actuator water_pump = { INIT_TTOBJECT_STATIC, "WaterPump",  ACTUATOR_OFF, 0 };
Actuator lamp       = { INIT_TTOBJECT_STATIC, "Lamp",       ACTUATOR_OFF, 0 };

static env_result_t Actuator_set(Actuator *self, ActuatorCmd *cmd)
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

static env_result_t Fan_set(tt_object_t *self, void *arg)
{
    return Actuator_set((Actuator *)self, (ActuatorCmd *)arg);
}

static env_result_t Pump_set(tt_object_t *self, void *arg)
{
    return Actuator_set((Actuator *)self, (ActuatorCmd *)arg);
}

static env_result_t Lamp_set(tt_object_t *self, void *arg)
{
    return Actuator_set((Actuator *)self, (ActuatorCmd *)arg);
}

void actuators_init(void)
{
}

void actuators_command_async(Actuator *a, int on)
{
    ActuatorCmd cmd = { .on = on };
    if (a == &fan)
        ASYNC(&fan, Fan_set, &cmd);
    else if (a == &water_pump)
        ASYNC(&water_pump, Pump_set, &cmd);
    else
        ASYNC(&lamp, Lamp_set, &cmd);
}

void actuators_set_fan_failed(int failed)
{
    fan.failed = failed ? 1 : 0;
    if (failed)
        log_event(LOG_WARN, "Fan", "actuator failure injected (stuck)");
}
