#include "global.h"
#include "util/util.h"

#define action_condition_text(condition)                   #condition
#define action_begin(status, condition)                    _action_begin(status, condition, action_condition_text(condition), __func__)
#define action_end(result)                                 _action_end(result, __func__)
#define action_command_execute(status, condition, command) _action_command_execute(status, condition, action_condition_text(condition), command, __func__)

int _action_begin(g_status_t status, int condition, const char *condition_text, const char *func) {
	if (!condition) return -1;
	log_info("executing action \"%s\"", func);
	log_info("condition: %s", condition_text);
	char text[ADMIN_INFO_SIZE];
	if (!g_status_text(text, sizeof(text), status))
		log_info("%s", text);
	return 0;
}

void _action_end(int result, const char *func) {
	if (!result)
		log_info("action \"%s\" executed successfully",func);
	else
		log_warn(9006, func);
}

void _action_command_execute(g_status_t status, int condition, const char *condition_text, const char *command, const char *func) {
	if (!command[0] || _action_begin(status, condition, condition_text, func)) return;
	_action_end(thread_unix_command_execute(command, NULL, -1, 1), func);
}

void action_master_vip_auto_down_execute(g_status_t status) {
	action_command_execute(status, status.master_vip_auto_down==G_VIP_AUTO_DOWN_NOT_EXECUTING, g_command_master_vip_auto_down_execute);
}

void action_master_vip_up(g_status_t status) {
	action_command_execute(
		status,
		status.standby_db_state!=G_DB_STATE_READ_WRITE && status.master_db_state==G_DB_STATE_READ_WRITE && status.master_vip_state==G_VIP_STATE_DOWN && status.master_vip_auto_down==G_VIP_AUTO_DOWN_EXECUTING,
		g_command_master_vip_up
	);
}

void action_master_vip_down(g_status_t status) {
	action_command_execute(status, status.master_vip_state==G_VIP_STATE_UP && (status.master_db_state!=G_DB_STATE_READ_WRITE || status.standby_db_state==G_DB_STATE_READ_WRITE), g_command_master_vip_down);
}

void action_standby_vip_auto_down_execute(g_status_t status) {
	action_command_execute(status, status.standby_vip_auto_down==G_VIP_AUTO_DOWN_NOT_EXECUTING, g_command_standby_vip_auto_down_execute);
}

void action_standby_vip_up(g_status_t status) {
	action_command_execute(
		status,
		status.master_vip_state!=G_VIP_STATE_UP && status.standby_db_state==G_DB_STATE_READ_WRITE && status.standby_vip_state==G_VIP_STATE_DOWN && status.standby_vip_auto_down==G_VIP_AUTO_DOWN_EXECUTING,
		g_command_standby_vip_up
	);
}

void action_standby_vip_down(g_status_t status) {
	return action_command_execute(status, status.standby_vip_state==G_VIP_STATE_UP && status.standby_db_state!=G_DB_STATE_READ_WRITE, g_command_standby_vip_down);
}

void action_standby_db_promote(g_status_t status) {
	if (action_begin(
		status,
		status.master_db_state!=G_DB_STATE_READ_WRITE && status.standby_db_state==G_DB_STATE_IN_RECOVERY && (time_now()-status.value_time)>g_time_standby_promote_delay_int && status.standby_db_lag<=g_time_standby_allowable_lag_int
	))
		return;
	thread_unix_command_execute(g_command_master_vip_down, NULL, -1, 1); // just try
	action_end(
		thread_unix_command_execute(g_command_standby_db_promote, NULL, -1, 1) ||
		thread_unix_command_execute(g_command_standby_vip_up,     NULL, -1, 1)
	);
}

void action_master_db_break(g_status_t status) {
	action_command_execute(status, status.master_db_state==G_DB_STATE_READ_WRITE && status.standby_db_state==G_DB_STATE_READ_WRITE, g_command_master_db_break);
}

void (*actions[])(g_status_t pgvip_status) = {
	action_master_vip_auto_down_execute,
	action_master_vip_up,
	action_master_vip_down,
	action_standby_vip_auto_down_execute,
	action_standby_vip_up,
	action_standby_vip_down,
	action_standby_db_promote,
	action_master_db_break
};

void* action_executor_thread(thread_params_t *params) {
	thread_begin("ACTION_EXECUTOR");
	g_status_t status;
	for(;;sleep(g_time_command_ssh_timeout_int+g_time_check_interval_int+1)) {
		for(int i=0; i<sizeof(actions)/sizeof(actions[0]); i++) {
			g_status_get(&status);
			if (status.value==G_STATUS_SUCCESS) break;
			actions[i](status);
		}
	}
	thread_end();
}
