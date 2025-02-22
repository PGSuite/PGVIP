#include <time.h>

#include "util.h"

time_t time_now() {
	time_t t;
	time(&t);
	return t;
}

int time_interval_str_add(char *str, int str_size, int interval) {
	return str_add_format(str, str_size, "%d %02d:%02d:%02d", interval/60/60/24, (interval/60/60)%24, (interval/60)%60, interval%60);
}

