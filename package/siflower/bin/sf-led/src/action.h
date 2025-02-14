#ifndef __ACTION_H_
#define __ACTION_H_

#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define DEBUG LOG_DEBUG

#define debug(level, fmt, ...) do { \
	if (level <= LOG_ERR) { \
		printf(fmt, ## __VA_ARGS__); \
	} else if (level <= DEBUG) {	\
		syslog(level, "sf-led %s[%d]" fmt, __func__, __LINE__, ## __VA_ARGS__); \
	} } while (0)

/*
* @LED_ACTION_NONE:	not used
* @LED_ACTION_1:	set led default on
* @LED_ACTION_2:	set led timer, delay_on:delay_off,each interval is 500ms
* @LED_ACTION_3:	set led none
* @LED_ACTION_4:	upgrade and reset mode : led-power timer and led-internet none
* @LED_ACTION_MAX:	not used
* */
enum {
	LED_ACTION_NONE,
	LED_ACTION_1,
	LED_ACTION_2,
	LED_ACTION_3,
	LED_ACTION_4,
	LED_ACTION_MAX,
};

/* global actions: all leds */
extern int led_set_trigger(const char *trig, int cnt, const char **name);

extern int set_led(const char *led_name, int action);

#endif /* __ACTION_H_ */
