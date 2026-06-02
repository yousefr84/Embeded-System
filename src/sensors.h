#ifndef SENSORS_H_
#define SENSORS_H_

#include "TinyTimber.h"

void sensors_init(void);
void sensors_set_temp_spike(int enabled);
void sensors_set_severe_noise(int enabled);
void sensors_set_heavy_load(int enabled);

#endif /* SENSORS_H_ */
