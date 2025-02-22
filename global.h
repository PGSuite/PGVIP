#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <limits.h>

#include "util/util.h"

#define G_CONFIG_FILE "/etc/pgvip.conf"

#define G_SIZE_IP              20
#define G_SIZE_COMMAND        500
#define G_SIZE_IFNAME          15
#define G_SIZE_TIME            10

#define G_STANDBY_LAG_UNKNOWN INT_MAX

typedef enum {
	G_STATUS_SUCCESS,
	G_STATUS_STANDBY_LAG,
	G_STATUS_STANDBY_PROMOTED,
	G_STATUS_ERROR
} g_status_value_t;

typedef enum {
	G_DB_STATE_READ_WRITE,
	G_DB_STATE_IN_RECOVERY,
    G_DB_STATE_NOT_AVAILABLE
} g_db_state_t;

typedef enum {
	G_VIP_STATE_UP,
	G_VIP_STATE_DOWN,
    G_VIP_STATE_NOT_AVAILABLE
} g_vip_state_t;

typedef enum {
	G_VIP_AUTO_DOWN_EXECUTING,
	G_VIP_AUTO_DOWN_NOT_EXECUTING,
    G_VIP_AUTO_DOWN_NOT_AVAILABLE
} g_vip_auto_down_t;


typedef struct {
	g_status_value_t value;
	time_t           value_time;

	g_db_state_t      master_db_state;
	time_t            master_db_time;
	g_vip_state_t     master_vip_state;
	char              master_vip_ifname[G_SIZE_IFNAME];
	time_t            master_vip_time;
	g_vip_auto_down_t master_vip_auto_down;

	g_db_state_t      standby_db_state;
	int               standby_db_lag;
	time_t            standby_db_time;
	g_vip_state_t     standby_vip_state;
	char              standby_vip_ifname[G_SIZE_IFNAME];
	time_t            standby_vip_time;
	g_vip_auto_down_t standby_vip_auto_down;
} g_status_t;

extern g_status_t g_status;

extern char g_ip_master     [G_SIZE_IP];
extern char g_ip_standby    [G_SIZE_IP];
extern char g_ip_virtual    [G_SIZE_IP];
extern char g_ip_subnet_mask[G_SIZE_IP];

extern char g_command_ssh[G_SIZE_COMMAND];

extern char g_command_db_state  [G_SIZE_COMMAND];
extern char g_command_db_break  [G_SIZE_COMMAND];
extern char g_command_db_promote[G_SIZE_COMMAND];

extern char g_command_ifname               [G_SIZE_COMMAND];
extern char g_command_vip_state            [G_SIZE_COMMAND];
extern char g_command_vip_up               [G_SIZE_COMMAND];
extern char g_command_vip_down             [G_SIZE_COMMAND];
extern char g_command_vip_auto_down_script [G_SIZE_COMMAND];
extern char g_command_vip_auto_down_execute[G_SIZE_COMMAND];

extern char g_time_command_ssh_timeout      [G_SIZE_TIME];
extern char g_time_check_interval           [G_SIZE_TIME];
extern char g_time_check_not_available_delay[G_SIZE_TIME];
extern char g_time_standby_allowable_lag    [G_SIZE_TIME];
extern char g_time_standby_promote_delay    [G_SIZE_TIME];
extern char g_time_vip_auto_down_timeout    [G_SIZE_TIME];
extern char g_time_vip_auto_down_duration   [G_SIZE_TIME];
extern char g_time_monitoring_interval      [G_SIZE_TIME];

extern int g_time_command_ssh_timeout_int;
extern int g_time_check_interval_int;
extern int g_time_check_not_available_delay_int;
extern int g_time_standby_allowable_lag_int;
extern int g_time_standby_promote_delay_int;
extern int g_time_vip_auto_down_timeout_int;
extern int g_time_vip_auto_down_duration_int;
extern int g_time_monitoring_interval_int;

extern char g_command_master_db_state             [G_SIZE_COMMAND];
extern char g_command_master_db_break             [G_SIZE_COMMAND];
extern char g_command_master_vip_state            [G_SIZE_COMMAND];
extern char g_command_master_vip_up               [G_SIZE_COMMAND];
extern char g_command_master_vip_down             [G_SIZE_COMMAND];
extern char g_command_master_vip_auto_down_execute[G_SIZE_COMMAND];

extern char g_command_standby_db_state             [G_SIZE_COMMAND];
extern char g_command_standby_db_promote           [G_SIZE_COMMAND];
extern char g_command_standby_vip_state            [G_SIZE_COMMAND];
extern char g_command_standby_vip_up               [G_SIZE_COMMAND];
extern char g_command_standby_vip_down             [G_SIZE_COMMAND];
extern char g_command_standby_vip_auto_down_execute[G_SIZE_COMMAND];

#endif /* GLOBAL_H_ */
