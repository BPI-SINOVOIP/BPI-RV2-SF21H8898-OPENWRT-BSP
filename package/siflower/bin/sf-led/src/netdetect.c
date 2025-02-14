#include <stdbool.h>
#include <sys/wait.h>
#include <regex.h>
#include "action.h"

volatile sig_atomic_t stop_flag = 0;
static char *PING_BAIDU_CMD = "ping -c 1 -w 1 www.baidu.com";
static const char *led[2] = { "led-power", "led-internet" };
bool ping_success;

void sigint_handler(int signum)
{
	stop_flag = 1;
}

int run_cmd(char *cmd, char result[])
{
	FILE *pp = popen(cmd, "r");
	int iRet = 0;
	if (!pp) {
		return -1;
	}
	while (fgets(result, 256, pp) != NULL) {
		if (result[strlen(result) - 1] == '\n') {
			result[strlen(result) - 1] = '\0';
		}
	}
	iRet = pclose(pp);
	if (WIFEXITED(iRet) == 0)
		return -1;
	else
		return WEXITSTATUS(iRet);
}

int get_ping_delay(char *result)
{
	const char *pattern = "[^0-9]+([0-9]+).*";
	char str[10] = "";
	regex_t reg;
	regmatch_t value[2];
	int delay;
	if (regcomp(&reg, pattern, REG_EXTENDED) != 0)
		return -1;
	if (regexec(&reg, result, 2, value, 0) != 0)
		return -1;
	memcpy(str, result + value[1].rm_so, value[1].rm_eo - value[1].rm_so);
	delay = atoi(str);
	return delay;
}

bool PING(void *args)
{
	bool ping_success = false;
	char *cmd = (char *)args;
	char result[256] = "";

	int ret = run_cmd(cmd, result);
	if (ret == 0) {
		int delay = get_ping_delay(result);
		if (delay != -1)
			ping_success = true;
	}

	return ping_success;
}

void *work_thread(void *arg)
{
	while (!stop_flag) {
		ping_success = PING(PING_BAIDU_CMD);
		if (!ping_success) {
			led_set_trigger("default-on", 1, &led[0]);
			led_set_trigger("none", 1, &led[1]);
		} else {
			led_set_trigger("default-on", 1, &led[1]);
			led_set_trigger("none", 1, &led[0]);
		}
		sleep(2);
	}
	pthread_exit(NULL);
}

int main()
{
	pthread_t thread_id;
	int ret;
	debug(LOG_DEBUG, "Start netdetect\n");

	signal(SIGTERM, sigint_handler);

	ret = pthread_create(&thread_id, NULL, work_thread, NULL);
	if (ret != 0) {
		debug(LOG_DEBUG, "pthread_create\n");
		exit(EXIT_FAILURE);
	}

	ret = pthread_join(thread_id, NULL);
	if (ret != 0) {
		debug(LOG_DEBUG, "pthread_join\n");
		exit(EXIT_FAILURE);
	}

	debug(LOG_DEBUG, "End netdetect\n");

	return 0;
}
