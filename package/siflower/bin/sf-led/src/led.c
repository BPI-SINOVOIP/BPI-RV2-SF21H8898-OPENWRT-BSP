#include "action.h"

pthread_mutex_t file_lock = PTHREAD_MUTEX_INITIALIZER;

static int led_config(const char *p, const char *q, int c, const char **s)
{
	int i, size;
	FILE *file;
	const char *path = "/sys/class/leds/";
	char filename[128];

	for (i = 0; i < c; i++) {
		strcpy(filename, path);
		strcat(filename, s[i]);
		strcat(filename, "/");
		strcat(filename, p);
		pthread_mutex_lock(&file_lock);
		file = fopen(filename, "w");
		if (file) {
			size = fwrite(q, sizeof(char), strlen(q), file);
			fclose(file);
			pthread_mutex_unlock(&file_lock);
			if (size != strlen(q))
				return -EBUSY;
		} else {
			pthread_mutex_unlock(&file_lock);
			return -EINVAL;
		}
	}

	return 0;
}

/* All leds in arg "name" must be available! */
int led_set_trigger(const char *trig, int cnt, const char **name)
{
	return led_config("trigger", trig, cnt, name);
}


int set_led(const char *led_name, int action)
{
	int ret;

	static const char *led[2] = { "led-power", "led-internet" };
	debug(LOG_DEBUG, "led %s action %d\n", led_name, action);

	switch (action) {
	case LED_ACTION_1:
		led_set_trigger("default-on", 1, &led_name);
		break;
	case LED_ACTION_2:
		led_set_trigger("timer", 1, &led_name);
		break;
	case LED_ACTION_3:
		led_set_trigger("none", 1, &led_name);
		break;
	case LED_ACTION_4:
		led_set_trigger("none", 1, &led[1]);
		led_set_trigger("timer", 1, &led[0]);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}
