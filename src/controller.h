#ifndef CONTROLLER_H_
#define CONTROLLER_H_

#include "TinyTimber.h"
#include <stdint.h>

typedef struct {
    OBJECT_HEAD;
    float temperature_c;
    float humidity_pct;
    int   light_level;
    int   fan_cmd;
    int   pump_cmd;
    int   lamp_cmd;
    uint32_t tick_count;
} Controller;

extern Controller greenhouse_controller;

void controller_init(void);
void controller_notify_temp(float value);
void controller_notify_humidity(float value);
void controller_notify_light(int value);

#endif /* CONTROLLER_H_ */
