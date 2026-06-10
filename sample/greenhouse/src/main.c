#include <tT.h>
#include <env.h>

#include "argv.h"

void init(void);

/*
 * Same lifecycle as ENV_STARTUP(init), extended with argc/argv for
 * verification scenario selection on the POSIX simulator.
 */
int main(int argc, char **argv)
{
	greenhouse_set_argv(argc, argv);
	tt_init();
	init();
	tt_run();
	return 0;
}
