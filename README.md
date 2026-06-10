# Real-Time Greenhouse — TinyTimber (POSIX)

Software greenhouse monitoring and control using the **official TinyTimber kernel** (`kernel/`) and the same project layout as the instructor samples (`tt/`, `tt2/`).

## Requirements

- GCC (C99), GNU Make, Linux
- pthreads and librt (`-lpthread -lrt`)

## Project structure

```
.
├── Makefile                      # convenience: make -C sample/greenhouse
├── README.md
├── REPORT.md
├── kernel/
│   ├── tT/                       # TinyTimber kernel (tT.h, kernel.c)
│   └── env/posix/                # POSIX simulator environment
└── sample/greenhouse/            # application (extends tt2/sample/controller)
    ├── Makefile
    └── src/
        ├── main.c                # tt_init(); init(); tt_run();
        ├── config.h              # PERIOD_* and thresholds
        ├── controller.c, sensors.c, actuators.c, scenarios.c, log.c
        └── Makefile
```

Instructor reference trees (not modified): `tt/`, `tt2/`.

## Build

From the repository root:

```bash
make
```

Or from the application directory (same as `tt2/sample/controller`):

```bash
cd sample/greenhouse
make
```

Produces `sample/greenhouse/posix/greenhouse.elf`.

## Run

```bash
./sample/greenhouse/posix/greenhouse.elf normal
./sample/greenhouse/posix/greenhouse.elf temp_spike
./sample/greenhouse/posix/greenhouse.elf severe_noise
./sample/greenhouse/posix/greenhouse.elf heavy_load
./sample/greenhouse/posix/greenhouse.elf fan_failure
./sample/greenhouse/posix/greenhouse.elf all
```

```bash
make run
make run-all
```

Each single scenario runs for **12 seconds** then exits (`TT_AFTER(ENV_SEC(12), …)`).

## Clean

```bash
make clean
```

## TinyTimber usage (same as instructor sample)

Application code uses official macros from `<tT.h>` and time helpers from `<env.h>`:

| Macro | Role |
|-------|------|
| `TT_ASYNC` | Non-blocking message |
| `TT_AFTER(ENV_MSEC(n), …)` | Periodic / delayed message |
| `TT_WITHIN(ENV_MSEC(bl), ENV_MSEC(dl), …)` | Baseline + deadline |
| `TT_ARGS_NONE` | No method argument |
| `tt_object()` | Static object initializer |

Methods: `static env_result_t handler(type_t *self, void *arg)`.

Full analysis and relation to sample code: **REPORT.md**.
