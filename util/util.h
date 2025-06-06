#ifndef UTIL_H_
#define UTIL_H_

#include <limits.h>
#include <string.h>

#define VERSION "25.2"

// #define TRACE 1

#define _util_var_name(name) #name

// file --------------------------------------------------------------------------------------------

#ifdef _WIN32
#define FILE_SEPARATOR  "\\"
#define PATH_SEPARATOR  ";"
#define OS_NAME         "windows"
#else
#define FILE_SEPARATOR  "/"
#define PATH_SEPARATOR  ":"
#define OS_NAME         "linux"
#endif

// log ---------------------------------------------------------------------------------------------

typedef enum {LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARN, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_TRACE} log_level;

#define LOG_TEXT_SIZE 1024

#define LOG_ERROR_NOT_FOUND_CURRENT_THREAD_CODE 58
#define LOG_ERROR_NOT_FOUND_CURRENT_THREAD_TEXT "Not found current thread"

#ifdef TRACE
	#define log_trace(format, ...) _log_trace(__FILE__, __func__, __LINE__, format, ##__VA_ARGS__)
#else
	#define log_trace(format, ...)
#endif

// log_error always returns 1, log_warn - -1
#define log_error(code, ...)                     _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_ERROR, code, NULL, 0,           ##__VA_ARGS__)
#define log_warn(code, ...)                      _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_WARN,  code, NULL, 0,           ##__VA_ARGS__)
#define log_error_errno(code, errno_value, ...)  _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_ERROR, code, NULL, errno_value, ##__VA_ARGS__)
#define log_warn_errno(code, errno_value, ...)   _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_WARN,  code, NULL, errno_value, ##__VA_ARGS__)

#ifdef _WIN32
#define log_error_errno_tcp(code, errno_tcp_value, ...)  _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_ERROR, code, "WSA error", errno_tcp_value, ##__VA_ARGS__)
#define log_warn_errno_tcp(code,  errno_tcp_value, ...)  _log_code(__FILE__, __func__, __LINE__, LOG_LEVEL_WARN,  code, "WSA error", errno_tcp_value, ##__VA_ARGS__)
#else
#define log_error_errno_tcp log_error_errno
#define log_warn_errno_tcp  log_warn_errno
#endif


// tcp ---------------------------------------------------------------------------------------------

#ifdef _WIN32

#include <winsock2.h>

#define tcp_socket      SOCKET
#define tcp_errno       WSAGetLastError()
#define TCP_SEND_FLAGS  0

#else

#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>

#define tcp_socket      int
#define tcp_errno       errno
#define TCP_SEND_FLAGS  MSG_NOSIGNAL

#endif

#define TCP_TIMEOUT 5

extern char tcp_host_name[256];
extern char tcp_host_addr[32];

// stream ---------------------------------------------------------------------------------------

typedef struct
{
	int	 len;
	int	 size;
	char *data;
	int last_add_str_unescaped;
} stream;

typedef struct
{
	int	 len;
	char names[100][128];
	stream streams[100];
	char *data[100];
} stream_list;

// thread ------------------------------------------------------------------------------------------

#ifdef _WIN32

#include <processthreadsapi.h>
#define thread_sys_id  HANDLE
#define thread_mutex_t HANDLE

#else

#include <pthread.h>
#define thread_sys_id  pthread_t
#define thread_mutex_t pthread_mutex_t

#endif

#define THREAD_NAME_SIZE  20

typedef struct {
	char name[THREAD_NAME_SIZE];
	#ifdef TRACE
		int mem_allocated;
	#endif
} thread_info_t;

typedef struct {
	tcp_socket socket_connection;
} thread_params_t;

extern unsigned char threads_initialized;

#define thread_mutex_init(p_mutex)   _thread_mutex_init(p_mutex, _util_var_name(p_mutex))
#define thread_semaphore_init(p_sem) _thread_semaphore_init(p_sem, _util_var_name(p_sem))

#ifndef TRACE

#define thread_mem_check_leak()

#endif


// str ---------------------------------------------------------------------------------------------

#define STR_SIZE 1024

#define STR_COLLECTION_SIZE        30
#define STR_COLLECTION_KEY_SIZE    50
#define STR_COLLECTION_VALUE_SIZE 200

typedef struct
{
	int	 len;
	char keys  [STR_COLLECTION_SIZE][STR_COLLECTION_KEY_SIZE];
	char values[STR_COLLECTION_SIZE][STR_COLLECTION_VALUE_SIZE];
} str_map;

typedef struct
{
	int	 len;
	char values[STR_COLLECTION_SIZE][STR_COLLECTION_VALUE_SIZE];
} str_list;

// admin --------------------------------------------------------------------------------------------

#define ADMIN_INFO_SIZE 10*1024

typedef int admin_command_function_t(char *info, int info_size);

#endif /* UTIL_H_ */
