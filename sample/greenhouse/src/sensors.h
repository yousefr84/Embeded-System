#ifndef GREENHOUSE_SENSORS_H_
#define GREENHOUSE_SENSORS_H_

void sensors_init(void);
void sensors_set_temp_spike(int enabled);
void sensors_set_severe_noise(int enabled);
void sensors_set_heavy_load(int enabled);

#endif /* GREENHOUSE_SENSORS_H_ */
