#include "global.h"
#include "util/util.h"

void* monitoring_thread(thread_params_t *params) {
	thread_begin("MONITORING");
	int interval = g_time_command_ssh_timeout_int+g_time_check_interval_int+1;
	g_status_value_t status_value_notified = G_STATUS_SUCCESS;
	for(sleep(interval*3);;sleep(interval)) {
		g_status_t status;
		g_status_get(&status);
		char command[G_SIZE_COMMAND];
		if (g_command_monitoring_send[0]) {
			if (
				str_copy(command, sizeof(command), g_command_monitoring_send) ||
				str_replace(command, sizeof(command), "${status}", G_STATUS_NAMES[status.value])
			)
				continue;
			thread_unix_command_execute(command, NULL, -1, 0);
		}
		if (g_command_monitoring_notify[0] && status_value_notified!=status.value) {
			status_value_notified=status.value;
			if (
				str_copy(command, sizeof(command), g_command_monitoring_notify) ||
				str_replace(command, sizeof(command), "${status}", G_STATUS_NAMES[status.value])
			)
				continue;
			thread_unix_command_execute(command, NULL, -1, 1);
		}
	}
	thread_end();
}

