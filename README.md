# Real-Time Greenhouse — Official TinyTimber (POSIX)

Software-only greenhouse monitoring and control using the **official TinyTimber kernel** provided for the course (`kernel/`, Luleå University / [simait/TinyTimber](https://github.com/simait/TinyTimber)).

## Requirements

- GCC (C99), GNU Make, Linux
- pthreads and librt (`-pthread -lrt`)

## Project structure

```
.
├── Makefile
├── README.md
├── REPORT.md
├── include/
│   ├── TinyTimber.h      # Course API (SYNC, ASYNC, INSTALL, …) → official tT.h
│   ├── app_config.h
│   └── log.h
├── kernel/
│   ├── tT/                  # Official kernel (kernel.c, tT.h)
│   └── env/posix/           # POSIX environment
└── src/
    ├── main.c              # INSTALL(greenhouse_install)
    ├── sensors.c, controller.c, actuators.c, scenarios.c, log.c
    └── greenhouse_argv.c
```

## Official kernel files used

| Component | Path |
|-----------|------|
| TinyTimber API | `kernel/tT/tT.h`, `kernel.h` |
| Kernel implementation | `kernel/tT/kernel.c` |
| POSIX environment | `kernel/env/posix/env.c`, `ack.c` |

Application code includes `include/TinyTimber.h`, which pulls in `<env.h>`, `<kernel.h>`, `<tT.h>` and defines course macros (`SYNC` → `TT_SYNC`, `INSTALL` → `tt_init` + init + `tt_run`, etc.).

## Build

```bash
cd EmbdedSystem
make
```

Produces `greenhouse`.

## Run

```bash
./greenhouse normal
./greenhouse temp_spike
./greenhouse severe_noise
./greenhouse heavy_load
./greenhouse fan_failure
./greenhouse all          # runs each scenario in a separate process (~12 s each)
```

```bash
make run
make run-all
```

Each single scenario runs for **12 seconds** then exits (scheduled via `AFTER(ENV_SEC(12), …)`).

## Clean

```bash
make clean
```

## API (course macros → official)

| Course | Official (tT.h) |
|--------|------------------|
| `SYNC` | `TT_SYNC` |
| `ASYNC` | `TT_ASYNC` |
| `AFTER` | `TT_AFTER` |
| `BEFORE` | `TT_BEFORE` |
| `WITHIN` | `TT_WITHIN` |
| `NOARG` | `TT_ARGS_NONE` |
| `PeriodicTimer(ms, o, m)` | `TT_AFTER(ENV_MSEC(ms), o, m, TT_ARGS_NONE)` |
| `INSTALL(fn)` | `tt_init(); fn(); tt_run();` (same as `ENV_STARTUP`) |
| `OBJECT_HEAD` | `tt_object_t obj` |
| `INIT_TTOBJECT_STATIC` | `tt_object()` |

Methods use the official signature: `env_result_t method(tt_object_t *self, void *arg)`.

Full analysis: **REPORT.md**.
