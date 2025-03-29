#include "global.h"
#include "util/util.h"

void* checker_master_db_thread(thread_params_t *params) {
	thread_begin("CHECKER_MASTER_DB");
	char output[200];
	for(;;sleep(g_time_check_interval_int)) {
	    int result = thread_unix_command_execute(g_command_master_db_state, output, sizeof(output), 0);
		time_t now = time_now();
		g_status_lock();
		if (!result && (output[0]=='t' || output[0]=='f')) {
			g_status.master_db_state = output[0]=='t' ? G_DB_STATE_IN_RECOVERY : G_DB_STATE_READ_WRITE;
			g_status.master_db_time = now;
		} else {
			if (g_status.master_db_state!=G_DB_STATE_NOT_AVAILABLE && (now-g_status.master_db_time)>g_time_check_not_available_delay_int) {
				g_status.master_db_state = G_DB_STATE_NOT_AVAILABLE;
				g_status.master_db_time = now;
			}
		}
		g_status_unlock_refresh();
	}
	thread_end();
}

void* checker_standby_db_thread(thread_params_t *params) {
	thread_begin("CHECKER_STANDBY_DB");
	char output[200];
	for(;;sleep(g_time_check_interval_int)) {
	    int result = thread_unix_command_execute(g_command_standby_db_state, output, sizeof(output), 0);
		time_t now = time_now();
		g_status_lock();
		if (!result && (output[0]=='t' || output[0]=='f') && output[1]=='|') {
			g_status.standby_db_state = output[0]=='t' ? G_DB_STATE_IN_RECOVERY : G_DB_STATE_READ_WRITE;
			if (output[0]=='t') {
				g_status.standby_db_state = G_DB_STATE_IN_RECOVERY;
				g_status.standby_db_lag = output[2] ? atoi(output+2) : G_STANDBY_LAG_UNKNOWN;
			} else
				g_status.standby_db_state = G_DB_STATE_READ_WRITE;
			g_status.standby_db_time = now;
		} else {
			if (g_status.standby_db_state!=G_DB_STATE_NOT_AVAILABLE && (now-g_status.standby_db_time)>g_time_check_not_available_delay_int) {
				g_status.standby_db_state = G_DB_STATE_NOT_AVAILABLE;
				g_status.standby_db_time = now;
			}
		}
		g_status_unlock_refresh();
	}
	thread_end();
}

void checker_vip(char *command_state, g_vip_state_t *state, char *ifname, g_vip_auto_down_t *auto_down_state, time_t *state_time) {
	char output[200];
	for(;;sleep(g_time_check_interval_int)) {
		int result = thread_unix_command_execute(command_state, output, sizeof(output), 0);
		str_replace_char(output, '\n', 0);
		time_t now = time_now();
		g_status_lock();
		if (!result && strlen(output)>=2 && (output[0]=='0' || output[0]=='1') && output[1]==',') {
			*state = output[2] ? G_VIP_STATE_UP : G_VIP_STATE_DOWN;
			*auto_down_state = output[0]=='1' ? G_VIP_AUTO_DOWN_EXECUTING : G_VIP_AUTO_DOWN_NOT_EXECUTING;
			strncpy(ifname, output+2, sizeof(g_status.master_vip_ifname));
		} else {
			if (*state!=G_VIP_STATE_NOT_AVAILABLE && (now-*state_time)>g_time_check_not_available_delay_int) {
				*state           = G_VIP_STATE_NOT_AVAILABLE;
				*auto_down_state = G_VIP_AUTO_DOWN_NOT_AVAILABLE;
				*state_time = now;
			}
		}
		g_status_unlock_refresh();
	}
}

void* checker_master_vip_thread(thread_params_t *params) {
	thread_begin("CHECKER_MASTER_VIP");
	checker_vip(g_command_master_vip_state, &g_status.master_vip_state, g_status.master_vip_ifname, &g_status.master_vip_auto_down, &g_status.master_vip_time);
	thread_end();
}

void* checker_standby_vip_thread(thread_params_t *params) {
	thread_begin("CHECKER_STANDBY_VIP");
	checker_vip(g_command_standby_vip_state, &g_status.standby_vip_state, g_status.standby_vip_ifname, &g_status.standby_vip_auto_down, &g_status.standby_vip_time);
	thread_end();
}
