#include "log.h"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static const char *scenario;
static uint32_t info_count;
static uint32_t warn_count;
static uint32_t miss_count;
static struct timespec log_start;
static int log_start_set;

static uint32_t ms_since_start(void)
{
	struct timespec now;
	uint64_t a, b;
	if (!log_start_set)
		return 0;
	clock_gettime(CLOCK_MONOTONIC, &now);
	a = (uint64_t)now.tv_sec * 1000ULL + (uint64_t)now.tv_nsec / 1000000ULL;
	b = (uint64_t)log_start.tv_sec * 1000ULL + (uint64_t)log_start.tv_nsec / 1000000ULL;
	return (uint32_t)(a - b);
}

void log_init(const char *scenario_name)
{
	scenario = scenario_name ? scenario_name : "unknown";
	info_count = warn_count = miss_count = 0;
	clock_gettime(CLOCK_MONOTONIC, &log_start);
	log_start_set = 1;
	printf("=== Greenhouse RT Simulation | scenario: %s ===\n", scenario);
	fflush(stdout);
}

void log_event(log_level_t level, const char *component, const char *fmt, ...)
{
	va_list ap;
	const char *tag = "INFO";

	switch (level) {
	case LOG_WARN: tag = "WARN"; warn_count++; break;
	case LOG_ERR:  tag = "ERR "; break;
	case LOG_DL_MISS: tag = "MISS"; miss_count++; break;
	default: info_count++; break;
	}

	printf("[%7u ms][%-5s][%-10s] ", ms_since_start(), tag, component);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	putchar('\n');
	fflush(stdout);
}

void log_deadline_miss(const char *task, uint32_t now_ms, uint32_t deadline_ms,
                       uint32_t finish_ms)
{
	log_event(LOG_DL_MISS, "SCHED",
	          "deadline miss: %s finish=%u deadline=%u (now=%u, late=%u ms)",
	          task, (unsigned)finish_ms, (unsigned)deadline_ms, (unsigned)now_ms,
	          (unsigned)(finish_ms - deadline_ms));
}

void log_stats_print(void)
{
	printf("--- log summary: info=%u warn=%u deadline_miss_log=%u ---\n",
	       (unsigned)info_count, (unsigned)warn_count, (unsigned)miss_count);
	fflush(stdout);
}
