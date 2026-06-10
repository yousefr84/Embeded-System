#ifndef GREENHOUSE_CONTROLLER_H_
#define GREENHOUSE_CONTROLLER_H_

#include <tT.h>
#include <stdint.h>

typedef struct {
	tt_object_t object;
	float temperature_c;
	float humidity_pct;
	int   light_level;
	int   fan_cmd;
	int   pump_cmd;
	int   lamp_cmd;
	uint32_t tick_count;
} controller_t;

extern controller_t greenhouse_controller;

void controller_init(void);
void controller_notify_temp(float value);
void controller_notify_humidity(float value);
void controller_notify_light(int value);

#endif /* GREENHOUSE_CONTROLLER_H_ */
