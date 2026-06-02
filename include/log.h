#ifndef LOG_H_
#define LOG_H_

#include <stdint.h>

typedef enum {
    LOG_INFO = 0,
    LOG_WARN,
    LOG_ERR,
    LOG_DL_MISS
} log_level_t;

void log_init(const char *scenario_name);
void log_event(log_level_t level, const char *component, const char *fmt, ...);
void log_stats_print(void);
void log_deadline_miss(const char *task, uint32_t now_ms, uint32_t deadline_ms,
                       uint32_t finish_ms);

#endif /* LOG_H_ */
