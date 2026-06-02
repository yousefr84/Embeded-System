# Real-Time Monitoring and Control System for a Greenhouse

**Course:** Real-Time and Embedded Systems  
**Runtime:** Official TinyTimber kernel (`kernel/`) with **env/posix** on Linux  

---

## 1. Project overview

This project implements a reactive real-time greenhouse simulator in C. All components are **TinyTimber objects** scheduled by the **official course kernel** (Luleå University TinyTimber, POSIX port). Virtual sensors, a periodic controller, and actuators communicate only through **asynchronous and timed messages** (`ASYNC`, `AFTER`, `WITHIN`, `PeriodicTimer`).

The previous custom simulation kernel was removed; the binary now links `tT/kernel.c` and `env/posix/env.c` from the professor-provided tree.

---

## 2. System architecture

```
  TempSensor     --ASYNC-->  Controller  --ASYNC-->  Fan / Pump / Lamp
  HumiditySensor --ASYNC-->      |
  LightSensor    --ASYNC-->      +-- PeriodicTimer(100ms) Controller_tick
```

Startup follows official TinyTimber POSIX flow:

1. `tt_init()` — kernel and environment initialization  
2. `greenhouse_install()` — create objects, schedule first messages  
3. `tt_run()` — interrupt-driven scheduler (timer thread + `tt_schedule()`)

---

## 3. Official TinyTimber design

### 3.1 Source tree

| Role | File |
|------|------|
| API | `kernel/tT/tT.h` |
| Kernel | `kernel/tT/kernel.c` |
| POSIX env | `kernel/env/posix/env.c`, `ack.c` |
| Course wrapper | `include/TinyTimber.h` (maps `SYNC` → `TT_SYNC`, etc.) |

### 3.2 Reactive objects

Every struct begins with `tt_object_t obj` (`OBJECT_HEAD` in the course header):

```c
typedef struct {
    OBJECT_HEAD;   /* tt_object_t obj */
    float base_temp;
    ...
} TempSensor;
```

Initialization: `INIT_TTOBJECT_STATIC` → `tt_object()` from `tT.h`.

### 3.3 Message passing (official semantics)

| Course macro | Official macro | Use in greenhouse |
|--------------|----------------|-------------------|
| `ASYNC` | `TT_ASYNC` | Sensor → controller, controller → actuator |
| `AFTER` / `PeriodicTimer` | `TT_AFTER` | Periodic sampling; sporadic light (random ms) |
| `WITHIN` | `TT_WITHIN` | Controller first tick with 100 ms deadline |
| `SYNC` | `TT_SYNC` | Available (actuators use `ASYNC`) |
| `NOARG` | `TT_ARGS_NONE` | Methods without payload |

Time baselines use **`ENV_MSEC(n)`** and **`ENV_SEC(n)`** from `env/posix/env.h` (struct `timespec`), not raw integers.

### 3.4 Why reactive messages support real-time behavior

- **Explicit timing:** `PeriodicTimer(100, …)` expands to `TT_AFTER(ENV_MSEC(100), …)`, encoding the 100 ms control period in the kernel queue.  
- **Object mutex:** The official kernel serializes methods per object (`owned_by` / `wanted_by` in `tt_object_t`).  
- **POSIX timer interrupts:** `env/posix` drives `tt_expired()` and `tt_schedule()` on real clock ticks.  
- **Decoupling:** `ASYNC` lets sensors and actuators progress without blocking the controller’s reaction chain.

This matches the Timber model (Lindgren et al., DATE 2008): concurrency through **time-bounded messages**, not ad-hoc pthreads per task.

### 3.5 INSTALL / startup

`INSTALL(greenhouse_install)` in `main.c` is equivalent to the official `ENV_STARTUP` pattern:

```c
tt_init();
greenhouse_install();  /* schedule sensors, controller, scenario hooks */
tt_run();
```

`greenhouse_install()` only registers messages; execution begins when `tt_run()` enters the POSIX scheduling loop.

---

## 4. Sensor models

| Sensor | Period / pattern | Method | To controller |
|--------|------------------|--------|---------------|
| Temperature | `PeriodicTimer(200, …)` | `TempSensor_sample` | `ASYNC(…, Controller_tempRx, &value)` |
| Humidity | `PeriodicTimer(500, …)` | `HumiditySensor_sample` | `ASYNC(…, Controller_humRx, &value)` |
| Light | `AFTER(ENV_MSEC(random 1000–4000), …)` | `LightSensor_event` | `ASYNC(…, Controller_lightRx, &level)` |

Noise and thresholds are implemented inside object methods in `sensors.c`.

---

## 5. Controller logic

`Controller_tick` every 100 ms:

- Fan ON if temperature > 28 °C  
- Pump ON if humidity < 35 %  
- Lamp ON if light < 40  

Actuator commands via `ASYNC` to `Fan_set`, `Pump_set`, `Lamp_set`.

---

## 6. Actuator models

`Actuator` objects; methods update state and log transitions. **Fan failure:** `failed` flag causes `Fan_set` to log and ignore commands.

---

## 7. Task / activity table

| Activity | Type | Period / MIT | Est. WCET | Deadline |
|----------|------|--------------|-----------|----------|
| `Controller_tick` | Periodic | 100 ms | 0.15 ms | 100 ms |
| `TempSensor_sample` | Periodic | 200 ms | 0.08 ms | 200 ms |
| `HumiditySensor_sample` | Periodic | 500 ms | 0.08 ms | 500 ms |
| `LightSensor_event` | Sporadic | MIT ≥ 1 s | 0.06 ms | 500 ms (local) |

---

## 8. Priority and scheduling analysis

The official POSIX kernel schedules by **message baseline and deadline**, not application-assigned RM priorities. Analytic RM/RTA (periods 100 / 200 / 500 ms, low WCET) still applies to the logical tasks and shows \(U \approx 0.002 \ll 0.779\) — easily schedulable if mapped to a fixed-priority thread implementation.

---

## 9. Verification scenarios

| Scenario | Mechanism |
|----------|-----------|
| `normal` | Default parameters, 12 s run |
| `temp_spike` | `AFTER(ENV_SEC(3), …)` raises base temperature |
| `severe_noise` | ±2 °C noise |
| `heavy_load` | Extra spin in sensor methods + `Scenario_loadGen` every 50 ms |
| `fan_failure` | Stuck fan + hot temperature |
| `all` | Subprocess per scenario (official kernel one `tt_run()` per process) |

End of run: `AFTER(ENV_SEC(12), …, Scenario_end)` → log summary → `exit(0)`.

---

## 10. Sample log (fan failure)

```
[  100 ms][INFO ][Controller] fan command -> ON (T=31.10 C)
[  100 ms][WARN ][Fan       ] FAILURE: command ignored (requested ON, stuck at OFF)
```

---

## 11. Limitations

- One `tt_run()` per process; scenario `all` re-invokes the executable.  
- Scenario length fixed at 12 s via timed `AFTER`.  
- Optional add-ons in `TinyTimber add-ons/` (STM32 SIO) are not used on Linux.  

---

## 12. Conclusion

The greenhouse project is **fully integrated with the official TinyTimber kernel** supplied for the course (`kernel/`, POSIX environment). Application code uses reactive objects and **`SYNC` / `ASYNC` / `AFTER` / `BEFORE` / `INSTALL`** via `include/TinyTimber.h`, matching `tT/tT.h` semantics. The system demonstrates periodic and sporadic real-time behaviour with traceable logs and documented schedulability analysis.

---

## References

- P. Lindgren et al., *TinyTimber, Reactive Objects in C for Real-Time Embedded Systems*, DATE 2008.  
- Official source: `kernel/` (simait/TinyTimber).  
- C. Liu and J. Layland, RM scheduling bound, 1973.
