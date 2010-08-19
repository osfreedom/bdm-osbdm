int print_screen(const char *format, ...);
int open_log(char *path);
void close_log(void);

#ifdef DETAILED_LOG_CAPABILITY
	void print_screen_level(unsigned char msg_level, const char *format, ...);
	void set_dbg_level(int new_dbg_level);
	void print_last_error_level(unsigned char msg_level,int error);
	void print_dump_level(unsigned char msg_level, unsigned char *data, unsigned int size);
	void vprint_screen_level(unsigned char msg_level, const char *format, va_list list);
#else
	#define print_screen_level(...)
	#define set_dbg_level(new_dbg_level)
	#define print_last_error_level(msg_level,error)
	#define print_dump_level(msg_level, data, size)
	#define vprint_screen_level(msg_level, format, list)
#endif

/* debugging levels:

0 - basic messages, start-up, shutdown...
10 - connection/disconnection of debugger
20 - commands
30 - commands & their parameters, results
40 - detailed execution of commands
50 - details including TCP/IP

*/

#define TCP_LOG_LEVEL_HIGH		10
#define CMD_LOG_LEVEL_HIGH		20
#define CMD_LOG_LEVEL_LOW		30
#define CMD_LOG_LEVEL_DETAILED	40
#define TCP_LOG_LEVEL_LOW		50

