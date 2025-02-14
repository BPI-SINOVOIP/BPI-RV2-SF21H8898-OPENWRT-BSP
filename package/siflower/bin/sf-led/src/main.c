/*
 * Led and button centralized dispatcher.
 * Author: nevermore.wang@siflower.com.cn
 */

#include "action.h"

static int usage(void)
{
	debug(LOG_ERR, "*****sf-led usage****\n");
	debug(LOG_ERR, "/$PATH/sf-led [ -l arg1 arg2]\n");
	debug(LOG_ERR, "-l $NAME $ACTION : setup led configuration with the action");
	debug(LOG_ERR, "defined in action.h");

	return 0;
}

int main(int argc, char **argv)
{
	int ch, ret = 0, action = -1;
	const char *name;

	while ((ch = getopt(argc, argv, "l:h:")) != -1) {
		switch (ch) {
		case 'h':
			usage();
			break;
		case 'l':
			name = optarg;
			action = atoi(optarg + strlen(optarg) + 1);
			debug(LOG_DEBUG, "led %s %d!\n", name, action);
			ret = set_led(name, action);
			break;
		default:
			usage();
			break;
		}
	}

	debug(LOG_DEBUG, "%s ret is %d\n", __func__, ret);

	return ret;
}
