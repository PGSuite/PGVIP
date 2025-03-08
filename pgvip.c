#include "global.h"
#include "util/util.h"

const char HELP[] =
	"All parameters (IP addresses, OS commands, timings) in the configuration file " G_CONFIG_FILE " \n" \
	"\n" \
	"Usage:\n" \
	"  pgvip {COMMAND}\n" \
	"\n" \
	"Commands:\n" \
	"  start        start execution in another process\n" \
	"  execute      execute in current process\n" \
	"  status       show status\n" \
	"  stop         stop execution\n" \
	"  show config  show runtime configuration\n" \
	"  help|man     print this help\n" \
	"\n" \
	"Examples:\n" \
	"  pgvip execute\n" \
	"  pgvip status\n" \
    "  pgvip show config\n";

extern void* checker_master_db_thread(void *args);
extern void* checker_master_vip_thread(void *args);
extern void* checker_standby_db_thread(void *args);
extern void* checker_standby_vip_thread(void *args);
extern void* action_executor_thread(void *args);

extern admin_command_function_t g_admin_status, g_admin_show_config;

int main(int argc, char *argv[])
{
	log_set_program_info("PGVIP", "PGVIP - auto failover to standby PostgreSQL server using VIP");
	log_check_help(argc, argv, HELP);
	admin_check_command(argc, argv, 0, (char *[]) {"status", "show config", NULL}, (admin_command_function_t *[]) {g_admin_status, g_admin_show_config, NULL});
	g_initialize();
	thread_initialize();
	log_initialize2(g_log_file, strlen("CHECKER_STANDBY_VIP"));
	log_print_header();
	if (
		log_thread_create(atoi(g_log_storage_days), !strcasecmp(g_log_check_updates,"on")) ||
		admin_thread_create()                                                              ||
		thread_create(checker_master_db_thread,   "CHECKER_MASTER_DB",   NULL)             ||
		thread_create(checker_master_vip_thread,  "CHECKER_MASTER_VIP",  NULL)             ||
		thread_create(checker_standby_db_thread,  "CHECKER_STANDBY_DB",  NULL)             ||
		thread_create(checker_standby_vip_thread, "CHECKER_STANDBY_VIP", NULL)             ||
		thread_create(action_executor_thread,     "ACTION_EXECUTOR",     NULL)
	)
		log_exit_fatal();
	while(1) sleep(UINT_MAX);
}
