#include "TinyTimber.h"

static int   gh_argc;
static char **gh_argv;

void greenhouse_set_argv(int argc, char **argv)
{
    gh_argc = argc;
    gh_argv = argv;
}

int greenhouse_get_argc(void)
{
    return gh_argc;
}

char **greenhouse_get_argv(void)
{
    return gh_argv;
}
