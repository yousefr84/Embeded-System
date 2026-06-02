/*
 * Application-wide constants for the greenhouse simulation.
 */
#ifndef APP_CONFIG_H_
#define APP_CONFIG_H_

/* Sensor periods (milliseconds) */
#define TEMP_PERIOD_MS       200U
#define HUMIDITY_PERIOD_MS   500U
#define LIGHT_MIN_MS         1000U
#define LIGHT_MAX_MS         4000U

/* Controller */
#define CONTROLLER_PERIOD_MS 100U
#define CONTROLLER_DEADLINE_MS CONTROLLER_PERIOD_MS

/* Control thresholds */
#define TEMP_FAN_ON_C        28.0f
#define HUMIDITY_PUMP_ON_PCT 35.0f
#define LIGHT_LAMP_ON_LEVEL  40

/* Noise */
#define TEMP_NOISE_NORMAL    0.5f
#define TEMP_NOISE_SEVERE    2.0f

/* Simulation horizon per scenario (milliseconds) */
#define SCENARIO_RUN_MS      12000U

/* Light sensor output range (integer, low range) */
#define LIGHT_MIN_VALUE      0
#define LIGHT_MAX_VALUE      100

#endif /* APP_CONFIG_H_ */
