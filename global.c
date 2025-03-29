#include <sys/time.h>

#include "global.h"
#include "util/util.h"

const char *G_STATUS_NAMES   []     = {"SUCCESS", "STANDBY_LAG", "STANDBY_PROMOTED", "ERROR"};
const char *G_DB_STATE_NAMES []     = {"read-write", "in recovery",   "not available"};
const char *G_VIP_STATE_NAMES[]     = {"up",         "down",          "not available"};
const char *G_VIP_AUTO_DOWN_NAMES[] = {"executing",  "not executing", "not available"};

g_status_t     g_status;
thread_mutex_t g_status_mutex;

char g_ip_master     [G_SIZE_IP] = "";
char g_ip_standby    [G_SIZE_IP] = "";
char g_ip_virtual    [G_SIZE_IP] = "";
char g_ip_subnet_mask[G_SIZE_IP] = "";

char g_command_ssh[G_SIZE_COMMAND] = "";

char g_command_db_state  [G_SIZE_COMMAND] = "";
char g_command_db_break  [G_SIZE_COMMAND] = "";
char g_command_db_promote[G_SIZE_COMMAND] = "";

char g_command_ifname               [G_SIZE_COMMAND] = "";
char g_command_vip_state            [G_SIZE_COMMAND] = "";
char g_command_vip_up               [G_SIZE_COMMAND] = "";
char g_command_vip_down             [G_SIZE_COMMAND] = "";
char g_command_vip_auto_down_script [G_SIZE_COMMAND] = "";
char g_command_vip_auto_down_execute[G_SIZE_COMMAND] = "";

char g_command_monitoring_send  [G_SIZE_COMMAND] = "";
char g_command_monitoring_notify[G_SIZE_COMMAND] = "";

char g_time_command_ssh_timeout      [G_SIZE_TIME] = "";
char g_time_check_interval           [G_SIZE_TIME] = "";
char g_time_check_not_available_delay[G_SIZE_TIME] = "";
char g_time_standby_allowable_lag    [G_SIZE_TIME] = "";
char g_time_standby_promote_delay    [G_SIZE_TIME] = "";
char g_time_vip_auto_down_timeout    [G_SIZE_TIME] = "";
char g_time_vip_auto_down_duration   [G_SIZE_TIME] = "";

int g_time_command_ssh_timeout_int;
int g_time_check_interval_int;
int g_time_check_not_available_delay_int;
int g_time_standby_allowable_lag_int;
int g_time_standby_promote_delay_int;
int g_time_vip_auto_down_timeout_int;
int g_time_vip_auto_down_duration_int;

char g_log_file         [PATH_MAX]         = "";
char g_log_storage_days [G_SIZE_LOG_PARAM] = "";
char g_log_check_updates[G_SIZE_LOG_PARAM] = "";

char g_command_master_db_state             [G_SIZE_COMMAND];
char g_command_master_db_break             [G_SIZE_COMMAND];
char g_command_master_vip_state            [G_SIZE_COMMAND];
char g_command_master_vip_up               [G_SIZE_COMMAND];
char g_command_master_vip_down             [G_SIZE_COMMAND];
char g_command_master_vip_auto_down_execute[G_SIZE_COMMAND];

char g_command_standby_db_state             [G_SIZE_COMMAND];
char g_command_standby_db_promote           [G_SIZE_COMMAND];
char g_command_standby_vip_state            [G_SIZE_COMMAND];
char g_command_standby_vip_up               [G_SIZE_COMMAND];
char g_command_standby_vip_down             [G_SIZE_COMMAND];
char g_command_standby_vip_auto_down_execute[G_SIZE_COMMAND];

#define g_var_name(g_var) #g_var
#define g_command_replace_var(g_var) str_format(var, sizeof(var), "${%s}", &(g_var_name(g_var))[2]) || str_replace(command, command_size, var, g_var)

void g_command_replace_vars(char *command, int command_size, char *g_ip) {
	char var[50];
	if (
		g_command_replace_var(g_command_vip_up)                 ||
		g_command_replace_var(g_command_vip_down)               ||
		g_command_replace_var(g_command_ifname)                 ||
		g_command_replace_var(g_ip)                             ||
		g_command_replace_var(g_ip_virtual)                     ||
		g_command_replace_var(g_ip_subnet_mask)                 ||
		g_command_replace_var(g_time_command_ssh_timeout)       ||
		g_command_replace_var(g_time_check_interval)            ||
		g_command_replace_var(g_time_check_not_available_delay) ||
		g_command_replace_var(g_time_standby_allowable_lag)     ||
		g_command_replace_var(g_time_standby_promote_delay)     ||
		g_command_replace_var(g_time_vip_auto_down_timeout)     ||
		g_command_replace_var(g_time_vip_auto_down_duration)
	)
		log_exit_fatal();
}

void g_command_replace_subcommand(char *command, int command_size, char *var, char *subcommand) {
	char r_var[100] = "";
	char r_subcommand[G_SIZE_COMMAND] = "";
	if (
		str_add(r_var, sizeof(r_var), " \"${", var, "}", NULL) ||
		str_copy(r_subcommand, sizeof(r_subcommand), subcommand) ||
		str_replace(r_subcommand, sizeof(r_subcommand), "\\", "\\\\") ||
		str_replace(r_subcommand, sizeof(r_subcommand), "\"", "\\\"") ||
		str_replace(r_subcommand, sizeof(r_subcommand), "`", "\\`") ||
		str_replace(r_subcommand, sizeof(r_subcommand), "$", "\\$") ||
		str_insert_char(r_subcommand, sizeof(r_subcommand), 0, '"') ||
		str_insert_char(r_subcommand, sizeof(r_subcommand), 0, ' ') ||
		str_replace(command, command_size, r_var, r_subcommand)
	)
		log_exit_fatal();
}

void g_command_ssh_build(char *command_final, int command_final_size, int master, char *command, char *ip) {
	char command_vars[G_SIZE_COMMAND];
	if (
		str_copy(command_final, command_final_size, g_command_ssh) ||
		str_copy(command_vars, sizeof(command_vars), command)
	)
		log_exit_fatal();
	g_command_replace_vars(command_vars, sizeof(command_vars), ip);
	g_command_replace_vars(command_final, command_final_size, master ? g_ip_master : g_ip_standby);
	g_command_replace_subcommand(command_final, command_final_size, "command", command_vars);
}

void g_command_vip_auto_down_execute_build(char *command_final, int command_final_size, int master) {
	char script[G_SIZE_COMMAND];
	char command[G_SIZE_COMMAND];
	if (
		str_copy(script, sizeof(script), g_command_vip_auto_down_script) ||
		str_copy(command, sizeof(command), g_command_vip_auto_down_execute)
	)
		log_exit_fatal();
	g_command_replace_vars(script, sizeof(script), g_ip_virtual);
	g_command_replace_subcommand(command, sizeof(command), "command_vip_auto_down_script", script);
	g_command_ssh_build(command_final, command_final_size, master, command, g_ip_virtual);
}

#define g_initialize_var_set(g_var) (!strcmp(param,&(g_var_name(g_var))[2]) && str_copy(g_var, sizeof(g_var), value))

void g_initialize() {

	stream config_body;
	if(file_read(G_CONFIG_FILE, &config_body)) log_exit_fatal();
	str_replace_char(config_body.data, '\r', ' ');
	str_replace_char(config_body.data, '\t', ' ');
	if(stream_add_char(&config_body, '\n')) log_exit_fatal();
	int pos_line_begin = 0;
	int pos_line_end;
	for(int pos_line_begin=0,pos_line_end;(pos_line_end=str_find_char(config_body.data, pos_line_begin,'\n'))!=-1;pos_line_begin=pos_line_end+1) {
		if (pos_line_end-pos_line_begin<3) continue;
		int pos_equals = str_find_char(config_body.data, pos_line_begin, '=');
		if (pos_equals==-1 || pos_equals>pos_line_end) continue;
		char param[G_SIZE_COMMAND];
		char value[G_SIZE_COMMAND];
		if (str_substr(param, sizeof(param), config_body.data, pos_line_begin, pos_equals-1) || str_substr(value, sizeof(value), config_body.data, pos_equals+1, pos_line_end-1)) log_exit_fatal();
		str_trim(param);
		if (param[0]=='#') continue;
		str_trim(value);
		if (
			g_initialize_var_set(g_ip_master)                      ||
			g_initialize_var_set(g_ip_standby)                     ||
			g_initialize_var_set(g_ip_virtual)                     ||
			g_initialize_var_set(g_ip_subnet_mask)                 ||
			g_initialize_var_set(g_command_ssh)                    ||
			g_initialize_var_set(g_command_db_state)               ||
			g_initialize_var_set(g_command_db_break)               ||
			g_initialize_var_set(g_command_db_promote)             ||
			g_initialize_var_set(g_command_ifname)                 ||
			g_initialize_var_set(g_command_vip_state)              ||
			g_initialize_var_set(g_command_vip_up)                 ||
			g_initialize_var_set(g_command_vip_down)               ||
			g_initialize_var_set(g_command_vip_auto_down_script)   ||
			g_initialize_var_set(g_command_vip_auto_down_execute)  ||
			g_initialize_var_set(g_command_monitoring_send)        ||
			g_initialize_var_set(g_command_monitoring_notify)      ||
			g_initialize_var_set(g_time_command_ssh_timeout)       ||
			g_initialize_var_set(g_time_check_interval)            ||
			g_initialize_var_set(g_time_check_not_available_delay) ||
			g_initialize_var_set(g_time_standby_allowable_lag)     ||
			g_initialize_var_set(g_time_standby_promote_delay)     ||
			g_initialize_var_set(g_time_vip_auto_down_timeout)     ||
			g_initialize_var_set(g_time_vip_auto_down_duration)    ||
			g_initialize_var_set(g_log_file)                       ||
			g_initialize_var_set(g_log_storage_days)               ||
			g_initialize_var_set(g_log_check_updates)
		)
			log_exit_fatal();
	}
	stream_free(&config_body);

	g_time_command_ssh_timeout_int       = atoi(g_time_command_ssh_timeout);
	g_time_check_interval_int            = atoi(g_time_check_interval);
	g_time_check_not_available_delay_int = atoi(g_time_check_not_available_delay);
	g_time_standby_allowable_lag_int     = atoi(g_time_standby_allowable_lag);
	g_time_standby_promote_delay_int     = atoi(g_time_standby_promote_delay);
	g_time_vip_auto_down_timeout_int     = atoi(g_time_vip_auto_down_timeout);
	g_time_vip_auto_down_duration_int    = atoi(g_time_vip_auto_down_duration);

	g_command_ssh_build(g_command_master_db_state,  sizeof(g_command_master_db_state),  1, g_command_db_state,  g_ip_master);
	g_command_ssh_build(g_command_master_db_break,  sizeof(g_command_master_db_break),  1, g_command_db_break,  g_ip_master);
	g_command_ssh_build(g_command_master_vip_state, sizeof(g_command_master_vip_state), 1, g_command_vip_state, g_ip_virtual);
	g_command_ssh_build(g_command_master_vip_up,    sizeof(g_command_master_vip_up),    1, g_command_vip_up,    g_ip_master);
	g_command_ssh_build(g_command_master_vip_down,  sizeof(g_command_master_vip_down),  1, g_command_vip_down,  g_ip_virtual);

	g_command_ssh_build(g_command_standby_db_state,   sizeof(g_command_standby_db_state),   0, g_command_db_state,   g_ip_standby);
	g_command_ssh_build(g_command_standby_db_promote, sizeof(g_command_standby_db_promote), 0, g_command_db_promote, g_ip_standby);
	g_command_ssh_build(g_command_standby_vip_state,  sizeof(g_command_standby_vip_state),  0, g_command_vip_state,  g_ip_virtual);
	g_command_ssh_build(g_command_standby_vip_up,     sizeof(g_command_standby_vip_up),     0, g_command_vip_up,     g_ip_standby);
	g_command_ssh_build(g_command_standby_vip_down,   sizeof(g_command_standby_vip_down),   0, g_command_vip_down,   g_ip_virtual);

	g_command_vip_auto_down_execute_build(g_command_master_vip_auto_down_execute,  sizeof(g_command_master_vip_auto_down_execute),  1);
	g_command_vip_auto_down_execute_build(g_command_standby_vip_auto_down_execute, sizeof(g_command_standby_vip_auto_down_execute), 0);

	if (thread_mutex_init(&g_status_mutex, "g_status_mutex")) log_exit_fatal();

	time_t now  = time_now();
	g_status.value      = G_STATUS_ERROR;
	g_status.value_time = now;
	g_status.master_db_state      = g_status.standby_db_state      = G_DB_STATE_NOT_AVAILABLE;
	g_status.master_db_time       = g_status.standby_db_time       = now;
	g_status.master_vip_state     = g_status.standby_vip_state     = G_VIP_STATE_NOT_AVAILABLE;
	g_status.master_vip_auto_down = g_status.standby_vip_auto_down = G_VIP_AUTO_DOWN_NOT_AVAILABLE;
	g_status.master_vip_time      = g_status.standby_vip_time      = now;

}

void g_status_lock()   { thread_mutex_lock  (&g_status_mutex); }
void g_status_unlock() { thread_mutex_unlock(&g_status_mutex); }

void g_status_unlock_refresh() {
	g_status_value_t status_value_old = g_status.value;
	if (
		g_status.master_db_state==G_DB_STATE_READ_WRITE  && g_status.master_vip_state==G_VIP_STATE_UP    && g_status.master_vip_auto_down==G_VIP_AUTO_DOWN_EXECUTING  &&
		g_status.standby_db_state==G_DB_STATE_IN_RECOVERY && g_status.standby_vip_state==G_VIP_STATE_DOWN && g_status.standby_vip_auto_down==G_VIP_AUTO_DOWN_EXECUTING
	) {
		g_status.value = g_status.standby_db_lag<(g_time_standby_allowable_lag_int/2) ? G_STATUS_SUCCESS : G_STATUS_STANDBY_LAG;
	} else if (
		g_status.master_vip_state!=G_VIP_STATE_UP &&
		g_status.standby_db_state==G_DB_STATE_READ_WRITE  && g_status.standby_vip_state==G_VIP_STATE_UP
	) {
		g_status.value = G_STATUS_STANDBY_PROMOTED;
	} else
		g_status.value = G_STATUS_ERROR;
	if (g_status.value!=status_value_old)
		g_status.value_time = time_now();
	g_status_unlock();
}

void g_status_get(g_status_t *pgvip_status) {
	g_status_lock();
	*pgvip_status = g_status;
	g_status_unlock();
}

int g_status_text(char *text, int text_size, g_status_t status) {
	char standby_db_state[100];  if (str_copy(standby_db_state,  sizeof(standby_db_state),  G_DB_STATE_NAMES[status.standby_db_state]))   return 1;
	char master_vip_state[100];  if (str_copy(master_vip_state,  sizeof(master_vip_state),  G_VIP_STATE_NAMES[status.master_vip_state]))  return 1;
	char standby_vip_state[100]; if (str_copy(standby_vip_state, sizeof(standby_vip_state), G_VIP_STATE_NAMES[status.standby_vip_state])) return 1;
	if (status.standby_db_state==G_DB_STATE_IN_RECOVERY) {
		if (status.standby_db_lag!=G_STANDBY_LAG_UNKNOWN) {
			if(str_add_format(standby_db_state, sizeof(standby_db_state), ", lag %d min.", status.standby_db_lag/60)) return 1;
		} else
			if(str_add(standby_db_state, sizeof(standby_db_state), ", unknown lag", NULL)) return 1;
	}
	if (status.master_vip_state  == G_VIP_STATE_UP && str_add(master_vip_state,  sizeof(master_vip_state),  " on ", status.master_vip_ifname,  NULL)) return 1;
	if (status.standby_vip_state == G_VIP_STATE_UP && str_add(standby_vip_state, sizeof(standby_vip_state), " on ", status.standby_vip_ifname, NULL)) return 1;
	char error_cause[100]="";
	if (status.value==G_STATUS_ERROR) {
		if (!error_cause[0] && status.master_db_state       != G_DB_STATE_READ_WRITE     && str_add(error_cause, sizeof(error_cause), "master db",             " is not ", G_DB_STATE_NAMES     [G_DB_STATE_READ_WRITE],     NULL)) return 1;
		if (!error_cause[0] && status.standby_db_state      != G_DB_STATE_IN_RECOVERY    && str_add(error_cause, sizeof(error_cause), "standby db",            " is not ", G_DB_STATE_NAMES     [G_DB_STATE_IN_RECOVERY],    NULL)) return 1;
		if (!error_cause[0] && status.master_vip_auto_down  != G_VIP_AUTO_DOWN_EXECUTING && str_add(error_cause, sizeof(error_cause), "master vip auto down",  " is not ", G_VIP_AUTO_DOWN_NAMES[G_VIP_AUTO_DOWN_EXECUTING], NULL)) return 1;
		if (!error_cause[0] && status.standby_vip_auto_down != G_VIP_AUTO_DOWN_EXECUTING && str_add(error_cause, sizeof(error_cause), "standby vip auto down", " is not ", G_VIP_AUTO_DOWN_NAMES[G_VIP_AUTO_DOWN_EXECUTING], NULL)) return 1;
		if (!error_cause[0] && status.master_vip_state      != G_VIP_STATE_UP            && str_add(error_cause, sizeof(error_cause), "master vip",            " is not ", G_VIP_STATE_NAMES    [G_VIP_STATE_UP],            NULL)) return 1;
		if (!error_cause[0] && status.standby_vip_state     != G_VIP_STATE_DOWN          && str_add(error_cause, sizeof(error_cause), "standby vip",           " is not ", G_VIP_STATE_NAMES    [G_VIP_STATE_DOWN],          NULL)) return 1;
	}
	const char caption_ip_address[]    = "IP address";
	const char caption_db_state[]      = "Database state";
	const char caption_vip_auto_down[] = "VIP auto down";
	int len_ip_address    = str_len_max(caption_ip_address,    g_ip_master,                                        g_ip_standby,                                        NULL);
	int len_db_state      = str_len_max(caption_db_state,      G_DB_STATE_NAMES[status.master_db_state],           standby_db_state,                                    NULL);
	int len_vip_state     = str_len_max(g_ip_virtual,          master_vip_state,                                   standby_vip_state,                                   NULL);
	int len_vip_auto_down = str_len_max(caption_vip_auto_down, G_VIP_AUTO_DOWN_NAMES[status.master_vip_auto_down], G_VIP_AUTO_DOWN_NAMES[status.standby_vip_auto_down], NULL);
	char line[STR_SIZE];
	if (str_format(line, sizeof(line), "+---------+-%*s-+-%*s-+-%*s-+-%*s-+\n", len_ip_address, "", len_db_state, "", len_vip_state, "", len_vip_auto_down, "")) return 1;
	str_replace_char(line, ' ', '-');
	return
		str_format(text, text_size, "status: %s (duration: ", G_STATUS_NAMES[status.value]) ||
		time_interval_str_add(text, text_size, time_now()-status.value_time) ||
		(error_cause[0] && str_add(text, text_size, ", cause: ", error_cause, NULL)) ||
		str_add(text, text_size, ")\n\n", line, NULL) ||
	    str_add_format(text, text_size, "| Role    | %-*s | %-*s | %-*s | %-*s |\n", len_ip_address, caption_ip_address, len_db_state, caption_db_state, len_vip_state, g_ip_virtual, len_vip_auto_down, caption_vip_auto_down) ||
		str_add(text, text_size, line, NULL) ||
	    str_add_format(text, text_size, "| master  | %-*s | %-*s | %-*s | %-*s |\n", len_ip_address, g_ip_master,  len_db_state, G_DB_STATE_NAMES[status.master_db_state], len_vip_state, master_vip_state,  len_vip_auto_down, G_VIP_AUTO_DOWN_NAMES[status.master_vip_auto_down])  ||
	    str_add_format(text, text_size, "| standby | %-*s | %-*s | %-*s | %-*s |\n", len_ip_address, g_ip_standby, len_db_state, standby_db_state,                         len_vip_state, standby_vip_state, len_vip_auto_down, G_VIP_AUTO_DOWN_NAMES[status.standby_vip_auto_down]) ||
		str_add(text, text_size, line, NULL);
}


int g_admin_status(char *info, int info_size) {
	g_status_t status;
	g_status_get(&status);
	return g_status_text(info, info_size, status);
}

#define g_admin_show_config_var_add(g_var) str_add_format(info, info_size, "  %-*s %*s\n" , var_name_len_max, &(g_var_name(g_var)":")[2], g_var_name(g_var)[2]=='t' ? 4 : 0, g_var)

int g_admin_show_config(char *info, int info_size) {
	int var_name_len_max;
	return
		log_get_header(info, info_size) ||
		str_add(info, info_size, "\nRuntime configuration\n\nIP addresses\n", NULL) || !(var_name_len_max=15) ||
		g_admin_show_config_var_add(g_ip_master)      ||
		g_admin_show_config_var_add(g_ip_standby)     ||
		g_admin_show_config_var_add(g_ip_virtual)     ||
		g_admin_show_config_var_add(g_ip_subnet_mask) ||
		str_add(info, info_size, "\nOS command templates\n", NULL) || !(var_name_len_max=30) ||
		g_admin_show_config_var_add(g_command_ssh)                   ||
		g_admin_show_config_var_add(g_command_db_state)              ||
		g_admin_show_config_var_add(g_command_db_break)              ||
		g_admin_show_config_var_add(g_command_db_promote)            ||
		g_admin_show_config_var_add(g_command_ifname)                ||
		g_admin_show_config_var_add(g_command_vip_state)             ||
		g_admin_show_config_var_add(g_command_vip_up)                ||
		g_admin_show_config_var_add(g_command_vip_down)              ||
		g_admin_show_config_var_add(g_command_vip_auto_down_script)  ||
		g_admin_show_config_var_add(g_command_vip_auto_down_execute) ||
		str_add(info, info_size, "\nMonitoring OS commands\n", NULL) || !(var_name_len_max=35) ||
		g_admin_show_config_var_add(g_command_monitoring_send)   ||
		g_admin_show_config_var_add(g_command_monitoring_notify) ||
		str_add(info, info_size, "\nTimings in seconds\n", NULL) || !(var_name_len_max=31) ||
		g_admin_show_config_var_add(g_time_command_ssh_timeout)       ||
		g_admin_show_config_var_add(g_time_check_interval)            ||
		g_admin_show_config_var_add(g_time_check_not_available_delay) ||
		g_admin_show_config_var_add(g_time_standby_allowable_lag)     ||
		g_admin_show_config_var_add(g_time_standby_promote_delay)     ||
		g_admin_show_config_var_add(g_time_vip_auto_down_timeout)     ||
		g_admin_show_config_var_add(g_time_vip_auto_down_duration)    ||
		str_add(info, info_size, "\nLogging\n", NULL) || !(var_name_len_max=18) ||
		g_admin_show_config_var_add(g_log_file)          ||
		g_admin_show_config_var_add(g_log_storage_days)  ||
		g_admin_show_config_var_add(g_log_check_updates) ||
		str_add(info, info_size, "\nGenerated OS commands\n", NULL) || !(var_name_len_max=38) ||
		g_admin_show_config_var_add(g_command_master_db_state)               ||
		g_admin_show_config_var_add(g_command_master_db_break)               ||
		g_admin_show_config_var_add(g_command_master_vip_state)              ||
		g_admin_show_config_var_add(g_command_master_vip_up)                 ||
		g_admin_show_config_var_add(g_command_master_vip_down)               ||
		g_admin_show_config_var_add(g_command_master_vip_auto_down_execute)  ||
		g_admin_show_config_var_add(g_command_standby_db_state)              ||
		g_admin_show_config_var_add(g_command_standby_db_promote)            ||
		g_admin_show_config_var_add(g_command_standby_vip_state)             ||
		g_admin_show_config_var_add(g_command_standby_vip_up)                ||
		g_admin_show_config_var_add(g_command_standby_vip_down)              ||
		g_admin_show_config_var_add(g_command_standby_vip_auto_down_execute);
}
