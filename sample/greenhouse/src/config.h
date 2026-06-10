#ifndef GREENHOUSE_CONFIG_H_
#define GREENHOUSE_CONFIG_H_

/* Sensor periods (milliseconds) */
#define PERIOD_TEMP       200U
#define PERIOD_HUM        500U
#define LIGHT_MIN_MS      1000U
#define LIGHT_MAX_MS      4000U

/* Controller */
#define PERIOD_CONTROLLER 100U
#define DEADLINE_CONTROLLER PERIOD_CONTROLLER

/* Control thresholds */
#define TEMP_FAN_ON       28.0f
#define HUM_PUMP_ON       35.0f
#define LIGHT_LAMP_ON     40

/* Noise */
#define TEMP_NOISE_NORMAL 0.5f
#define TEMP_NOISE_SEVERE 2.0f

/* Simulation horizon per scenario (milliseconds) */
#define SCENARIO_RUN_SEC  12U

/* Light sensor output range */
#define LIGHT_MIN_VALUE   0
#define LIGHT_MAX_VALUE   100

#endif /* GREENHOUSE_CONFIG_H_ */
