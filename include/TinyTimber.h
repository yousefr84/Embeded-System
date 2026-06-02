/*
 * Course-facing TinyTimber header — includes the official Luleå kernel
 * (kernel/tT/, kernel/env/posix/) and exposes SYNC, ASYNC, AFTER,
 * BEFORE, INSTALL, INIT_OBJECT, INIT_TTOBJECT as used in course materials.
 *
 * Official macros are TT_SYNC, TT_ASYNC, … in tT/tT.h.
 */
#ifndef GREENHOUSE_TINYTIMBER_H_
#define GREENHOUSE_TINYTIMBER_H_

#ifndef ENV_POSIX
#define ENV_POSIX 1
#endif

#include <env.h>
#include <kernel.h>
#include <tT.h>

/* ---------- Object layout (official: tt_object_t as first member) ---------- */

#define OBJECT_HEAD              tt_object_t obj
#define INIT_TTOBJECT            tt_object()
#define INIT_TTOBJECT_STATIC     tt_object()
#define INIT_OBJECT(type_name)   /* type_name: struct { OBJECT_HEAD; ... } */

/* ---------- Message passing (aliases of tT/tT.h) ---------- */

#define SYNC(to, meth, arg)           TT_SYNC(to, meth, arg)
#define ASYNC(to, meth, arg)          TT_ASYNC(to, meth, arg)
#define AFTER(bl, to, meth, arg)      TT_AFTER(bl, to, meth, arg)
#define BEFORE(dl, to, meth, arg)     TT_BEFORE(dl, to, meth, arg)
#define WITHIN(bl, dl, to, meth, arg) TT_WITHIN(bl, dl, to, meth, arg)
#define NOARG                         TT_ARGS_NONE

#define PeriodicTimer(period_ms, to, meth) \
    TT_AFTER(ENV_MSEC(period_ms), to, meth, TT_ARGS_NONE)

/* ---------- Startup (official: tt_init + init + tt_run via ENV_STARTUP) ---------- */

#define INSTALL(init_function)              \
    int main(int argc, char **argv)       \
    {                                     \
        greenhouse_set_argv(argc, argv);  \
        tt_init();                        \
        init_function();                  \
        tt_run();                         \
        return 0;                         \
    }

void greenhouse_set_argv(int argc, char **argv);
int  greenhouse_get_argc(void);
char **greenhouse_get_argv(void);

#endif /* GREENHOUSE_TINYTIMBER_H_ */
