# Real-Time Monitoring and Control System for a Greenhouse

**Course:** Real-Time and Embedded Systems  
**Framework:** TinyTimber (Luleå University of Technology) — `kernel/tT/`, `kernel/env/posix/`  
**Application:** `sample/greenhouse/` (extension of instructor sample in `tt/` / `tt2/`)

---

## 1. Introduction

This project implements a reactive real-time greenhouse simulator in C. The design follows the **TinyTimber reactive-object model** demonstrated in the instructor sample projects (`tt/example/controller`, `tt2/sample/controller`): each software component is a struct whose first member is `tt_object_t object`, and all interaction is expressed through **timed and asynchronous messages** (`TT_SYNC`, `TT_ASYNC`, `TT_AFTER`, `TT_WITHIN`).

The greenhouse application extends the sample controller pattern from a single monolithic actor to a **distributed object graph** (sensors, controller, actuators, scenario manager) while preserving the same engineering conventions: official kernel headers (`<tT.h>`, `<env.h>`), `ENV_MSEC` / `ENV_SEC` time baselines, static object initialization with `tt_object()`, and the POSIX startup sequence `tt_init()` → `init()` → `tt_run()`.

---

## 2. Relation to instructor sample code

| Aspect | Instructor sample (`tt` / `tt2`) | Greenhouse application |
|--------|----------------------------------|------------------------|
| Object layout | `tt_object_t object` first in struct | Same — all actors use `tt_object_t object` |
| Method signature | `static env_result_t handler(type_t *self, void *arg)` | Same — typed `self`, returns `0` |
| Periodic work | `TT_AFTER(ENV_MSEC(PERIOD), self, method, TT_ARGS_NONE)` | Same pattern for sensors and controller |
| Reactive chain | Sensor → `TT_ASYNC` → evaluate | Sensor → `TT_ASYNC` → controller receive methods |
| Startup | `ENV_STARTUP(init)` or equivalent | `main()` calls `tt_init(); init(); tt_run();` (with argc/argv for scenarios) |
| Build | Layered Makefiles: env + tT + app → `./posix/app.elf` | Same structure → `./posix/greenhouse.elf` |
| Constants | `#define PERIOD_TEMP …` at top of source | `config.h` with `PERIOD_TEMP`, `PERIOD_HUM`, etc. |

The sample controller combines sensors and control logic in one object. The greenhouse project **decomposes** that design into separate reactive objects linked by `TT_ASYNC`, which is the natural next step when adding actuators, logging, and verification scenarios without changing the underlying TinyTimber semantics.

---

## 3. System architecture

```
  temp_sensor      --TT_ASYNC-->  controller  --TT_ASYNC-->  fan / pump / lamp
  humidity_sensor  --TT_ASYNC-->      |
  light_sensor     --TT_ASYNC-->      +-- TT_AFTER(100 ms) controller_tick
```

### 3.1 Startup sequence

1. `tt_init()` — kernel and POSIX environment initialization  
2. `init()` — parse scenario, create objects, schedule first messages  
3. `tt_run()` — timer-driven scheduler (`tt_expired()` → `tt_schedule()`)

This matches the instructor sample lifecycle; scenario selection adds a thin argc/argv layer before `init()` (required for verification, not present in the minimal sample).

### 3.2 Source tree

| Role | Path |
|------|------|
| TinyTimber API | `kernel/tT/tT.h` |
| Kernel | `kernel/tT/kernel.c` |
| POSIX environment | `kernel/env/posix/env.c`, `ack.c` |
| Application | `sample/greenhouse/src/*.c` |
| Build entry | `sample/greenhouse/Makefile` |

Application code includes **only** `<tT.h>` and `<env.h>` for framework access (as in the sample). No custom macro wrapper layer is used.

---

## 4. Reactive objects and message passing

### 4.1 Object definition (sample pattern)

```c
typedef struct {
    tt_object_t object;
    float temperature_c;
    ...
} controller_t;

static controller_t greenhouse_controller = { tt_object(), 22.0f, ... };
```

### 4.2 Message macros (official semantics)

| Macro | Use in greenhouse |
|-------|---------------------|
| `TT_ASYNC` | Sensor → controller, controller → actuator |
| `TT_AFTER(ENV_MSEC(n), …)` | Periodic sampling (200 / 500 / 100 ms); sporadic light events |
| `TT_WITHIN(ENV_MSEC(bl), ENV_MSEC(dl), …)` | Controller first tick; heavy-load load generator |
| `TT_ARGS_NONE` | Methods without argument payload |

Time values use **`ENV_MSEC(n)`** and **`ENV_SEC(n)`** from `env/posix/env.h` (`struct timespec`), not raw integers.

### 4.3 Why reactive messages support real-time behavior

- **Explicit timing:** `TT_AFTER(ENV_MSEC(100), …)` encodes the 100 ms control period in the kernel message queue.  
- **Per-object serialization:** the kernel mutex (`owned_by` / `wanted_by` on `tt_object_t`) serializes methods on each object.  
- **POSIX timer thread:** `env/posix` drives `tt_expired()` and `tt_schedule()` on clock ticks.  
- **Decoupling:** `TT_ASYNC` lets sensors and actuators progress without blocking the controller chain.

This is the Timber model (Lindgren et al., DATE 2008): concurrency through **time-bounded messages**, not ad-hoc threads per task.

---

## 5. Sensor models

| Sensor | Period / pattern | Method | To controller |
|--------|------------------|--------|---------------|
| Temperature | `TT_AFTER(ENV_MSEC(200), …)` | `temp_sensor_sample` | `TT_ASYNC(…, controller_temp_rx, &value)` |
| Humidity | `TT_AFTER(ENV_MSEC(500), …)` | `humidity_sensor_sample` | `TT_ASYNC(…, controller_hum_rx, &value)` |
| Light | `TT_AFTER(ENV_MSEC(random 1000–4000), …)` | `light_sensor_event` | `TT_ASYNC(…, controller_light_rx, &level)` |

Noise, PRNG seeding, and scenario hooks are implemented inside sensor methods in `sensors.c`, following the sample’s approach of keeping simulation logic in the handler that also reschedules itself.

---

## 6. Controller logic

`controller_tick` every 100 ms (`PERIOD_CONTROLLER`):

- Fan ON if temperature > 28 °C (`TEMP_FAN_ON`)  
- Pump ON if humidity < 35 % (`HUM_PUMP_ON`)  
- Lamp ON if light < 40 (`LIGHT_LAMP_ON`)

Actuator commands via `TT_ASYNC` to `fan_set`, `pump_set`, `lamp_set` — the same **evaluate → act** split as `controller_evaluate` in the instructor sample, extended to three actuators.

First tick scheduled with:

```c
TT_WITHIN(ENV_MSEC(0), ENV_MSEC(DEADLINE_CONTROLLER),
          c, controller_tick, TT_ARGS_NONE);
```

Subsequent ticks reschedule with `TT_AFTER(ENV_MSEC(PERIOD_CONTROLLER), …)`.

---

## 7. Actuator models

Each actuator is a separate `actuator_t` object. Methods update state and log transitions. **Fan failure:** the `failed` flag causes `fan_set` to log and ignore commands (verification scenario).

---

## 8. Task / activity table

| Activity | Type | Period / MIT | Est. WCET | Deadline |
|----------|------|--------------|-----------|----------|
| `controller_tick` | Periodic | 100 ms | 0.15 ms | 100 ms |
| `temp_sensor_sample` | Periodic | 200 ms | 0.08 ms | 200 ms |
| `humidity_sensor_sample` | Periodic | 500 ms | 0.08 ms | 500 ms |
| `light_sensor_event` | Sporadic | MIT ≥ 1 s | 0.06 ms | 500 ms (local) |

---

## 9. Priority and scheduling analysis

The official POSIX kernel schedules by **message baseline and deadline**, not application-assigned RM priorities. Analytic RM/RTA (periods 100 / 200 / 500 ms, low WCET) still applies to the logical tasks and shows \(U \approx 0.002 \ll 0.779\) — easily schedulable if mapped to a fixed-priority thread implementation.

---

## 10. Verification scenarios

| Scenario | Mechanism |
|----------|-----------|
| `normal` | Default parameters, 12 s run |
| `temp_spike` | `TT_AFTER(ENV_SEC(3), …)` raises base temperature |
| `severe_noise` | ±2 °C noise |
| `heavy_load` | Extra spin in sensor methods + `scenario_load_gen` every 50 ms |
| `fan_failure` | Stuck fan + hot temperature |
| `all` | Subprocess per scenario (one `tt_run()` per process) |

End of run: `TT_AFTER(ENV_SEC(12), …, scenario_end)` → log summary → `exit(0)`.

Run from build directory:

```bash
cd sample/greenhouse && make
./posix/greenhouse.elf normal
./posix/greenhouse.elf all
```

---

## 11. Sample log (fan failure)

```
[  100 ms][INFO ][Controller] fan command -> ON (T=31.10 C)
[  100 ms][WARN ][Fan       ] FAILURE: command ignored (requested ON, stuck at OFF)
```

---

## 12. Build organization

The Makefile chain matches `tt2/sample/controller/Makefile`:

```
sample/greenhouse/Makefile
    ├── include kernel/env/posix/Makefile   → ENV_OBJECTS
    ├── include kernel/tT/Makefile          → TT_OBJECTS
    └── include src/Makefile                → APP_OBJECTS, greenhouse.elf
```

Output: `sample/greenhouse/posix/greenhouse.elf`. Top-level `make` delegates to this directory.

---

## 13. Limitations

- One `tt_run()` per process; scenario `all` re-invokes the executable.  
- Scenario length fixed at 12 s via timed `TT_AFTER`.  
- Log timestamps use `CLOCK_MONOTONIC` at print time (may differ by ±1 ms from scheduled instants under POSIX load).  

---

## 14. Conclusion

The greenhouse project is a **direct extension** of the instructor TinyTimber sample: same object model, message macros, startup flow, and layered build. Functionality is decomposed into multiple reactive objects and verification scenarios while preserving all control rules, timing parameters, schedulability analysis, and logging behavior required by the assignment.

---

## References

- P. Lindgren et al., *TinyTimber, Reactive Objects in C for Real-Time Embedded Systems*, DATE 2008.  
- Instructor sample: `tt/example/controller/src/main.c`, `tt2/sample/controller/src/main.c`.  
- Official kernel: `kernel/` (Luleå TinyTimber, POSIX port).  
- C. Liu and J. Layland, RM scheduling bound, 1973.
