#ifndef GREENHOUSE_ACTUATORS_H_
#define GREENHOUSE_ACTUATORS_H_

#include <tT.h>

typedef enum {
	ACTUATOR_OFF = 0,
	ACTUATOR_ON  = 1
} actuator_state_t;

typedef struct {
	tt_object_t object;
	const char *name;
	actuator_state_t state;
	int failed;
} actuator_t;

typedef struct {
	int on;
} actuator_cmd_t;

extern actuator_t fan;
extern actuator_t water_pump;
extern actuator_t lamp;

void actuators_init(void);
void actuators_command_async(actuator_t *a, int on);
void actuators_set_fan_failed(int failed);

#endif /* GREENHOUSE_ACTUATORS_H_ */
