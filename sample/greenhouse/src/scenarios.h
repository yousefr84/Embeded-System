#ifndef GREENHOUSE_SCENARIOS_H_
#define GREENHOUSE_SCENARIOS_H_

typedef enum {
	SCENARIO_NORMAL = 0,
	SCENARIO_TEMP_SPIKE,
	SCENARIO_SEVERE_NOISE,
	SCENARIO_HEAVY_LOAD,
	SCENARIO_FAN_FAILURE,
	SCENARIO_ALL
} scenario_id_t;

const char *scenario_name(scenario_id_t id);
scenario_id_t scenario_parse(const char *name);
void scenario_run(scenario_id_t id);

#endif /* GREENHOUSE_SCENARIOS_H_ */
