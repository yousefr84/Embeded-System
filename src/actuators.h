#ifndef ACTUATORS_H_
#define ACTUATORS_H_

#include "TinyTimber.h"

typedef enum {
    ACTUATOR_OFF = 0,
    ACTUATOR_ON  = 1
} actuator_state_t;

typedef struct {
    OBJECT_HEAD;
    const char *name;
    actuator_state_t state;
    int failed;
} Actuator;

typedef struct {
    int on;
} ActuatorCmd;

extern Actuator fan;
extern Actuator water_pump;
extern Actuator lamp;

void actuators_init(void);
void actuators_command_async(Actuator *a, int on);
void actuators_set_fan_failed(int failed);

#endif /* ACTUATORS_H_ */
