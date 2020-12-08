#include<stdio.h>
#include<sys/time.h>
#include<unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <stdlib.h>

#define LOG_MAX_ID_SIZE	(64)			/* max chars allowed in ID string */
#define LOG_MAX_FMT_STR_SIZE	(1024)	/* reasonable format string limit */
#define LOG_MIN_TIMESTAMP_SIZE	(24)	/* min buffer size for timestamp */
static char error_buf[1024];
static char fmt_buf[1024];
static char off_buf[80];

static unsigned al_log_options;        /* global log features mask */
enum al_log_options {
	LOG_OPT_NONE		= 0x0000,
	LOG_OPT_CONSOLE_OUT	= 0x0001,
	LOG_OPT_TIMESTAMPS	= 0x0002,
	LOG_OPT_FUNC_NAMES	= 0x0004,
	LOG_OPT_NO_SYSLOG	= 0x0008,
	LOG_OPT_DEBUG		= 0x0010
};
    typedef enum {
        QL_SYS_LOG_MIN = 0,
        QL_SYS_LOG_DEFAULT,    /* only for SetMinPriority() */
        QL_SYS_LOG_VERBOSE,
        QL_SYS_LOG_DEBUG,
        QL_SYS_LOG_INFO,
        QL_SYS_LOG_WARN,
        QL_SYS_LOG_ERROR,
        QL_SYS_LOG_FATAL,
        QL_SYS_LOG_SILENT,     /* only for SetMinPriority(); must be last */
        QL_SYS_LOG_MAX
    } QL_SYS_LOG_PRIORITY_E;

enum AL_Log_level
{
    LOG_AL_INFO,
     LOG_AL_DEBUG,
     LOG_AL_WARN,
     LOG_AL_ERR,

};
    typedef enum {
        QL_SYS_LOG_ID_MIN = -1,
        QL_SYS_LOG_ID_MAIN = 0,
        QL_SYS_LOG_ID_RADIO = 1,
        QL_SYS_LOG_ID_EVENTS = 2,
        QL_SYS_LOG_ID_SYSTEM = 3, 
        QL_SYS_LOG_ID_MAX
    } QL_SYS_LOG_ID_E;

    static FILE *log_get_console_stream(enum AL_Log_level level)
    {
        if (level >= LOG_AL_WARN) {
            return stderr;
        }
        return stdout;
    }

    static const char *AL_Log_get_console_tag(enum AL_Log_level level)
    {
        if (level <= LOG_AL_DEBUG) {
            return "[DBG]";
        }
        if (level <= LOG_AL_INFO) {
            return "[INF]";
        }
        if (level <= LOG_AL_WARN) {
            return "[WRN]";
        }
        return "[ERR]";
    }

    size_t AL_Log_get_timestamp(char *buf, size_t size)
    {
        struct timeval tv;
        struct tm cal;
        size_t len;
    
        gettimeofday(&tv, NULL);
        gmtime_r(&tv.tv_sec, &cal);
    
        len = snprintf(buf, size,
             "%04d-%02d-%02dT%02d:%02d:%02d.%03u",
              cal.tm_year + 1900,
              cal.tm_mon + 1,
              cal.tm_mday,
              cal.tm_hour+8,
              cal.tm_min,
              cal.tm_sec,
              (unsigned)(tv.tv_usec / 1000));
        if (len < size) {
            return len;
        }
        return size - 1;
    }


static void AL_Log_console_base(const char *func,
    enum AL_Log_level level,
    const char *fmt, va_list args)
{
    size_t len = 0;
    char fmt_buf[LOG_MAX_FMT_STR_SIZE];

    /* prepend with optional timestamp */
    if (al_log_options & LOG_OPT_TIMESTAMPS) {
        len += AL_Log_get_timestamp(fmt_buf, sizeof(fmt_buf));
        fmt_buf[len] = ' '; /* add trailing space */
        ++len;
    }

    /* add the level tag */
    len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
        "%s ", AL_Log_get_console_tag(level));

    /* optional function name */
    if ((al_log_options & LOG_OPT_FUNC_NAMES) && func) {
        len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len,
            "%s() ", func);
        if (len >= sizeof(fmt_buf)) {
            len = 0;
        }
    }

    /* complete format string */
    len += snprintf(fmt_buf + len, sizeof(fmt_buf) - len, " %s\n", fmt);

    if (len >= sizeof(fmt_buf)) {
        /* if fmt_buf is too small, just print the message */
        vfprintf(log_get_console_stream(level), fmt, args);
    } else {
        vfprintf(log_get_console_stream(level), fmt_buf, args);
    }
}

/*
 * Format and log a message to Syslog.  This is the default Syslog
 * logging function.
 *
 * func - the function name or NULL
 * level - log level
 * fmt, args - message format string and argument list
 */



/* console logging function */
static void (*al_log_console_func)(const char *, enum AL_Log_level,
    const char *, va_list) = AL_Log_console_base;


void AL_Log_base_subsystem(const char *func,
    enum AL_Log_level level,
    const char *tag,
    const char *fmt, ...)
{
    va_list args;

    /* ignore all messages at or below DEBUG level if debug is disabled */
    if (level <= LOG_AL_WARN && !(al_log_options & LOG_OPT_DEBUG)) {
        return;
    }
    
    /* log to the console */
    if (al_log_options & LOG_OPT_CONSOLE_OUT) {
        va_start(args, fmt);
        al_log_console_func(func, level, fmt, args);
        va_end(args);
    }
    /* log to Syslog */
    /*
    if (!(al_log_options & LOG_OPT_NO_SYSLOG)) {
        va_start(args, fmt);
        al_log_syslog_func(func, level, tag,fmt, args);
        va_end(args);
    }
    */
}
    void AL_Log_init(unsigned options)
    {
        al_log_console_func = AL_Log_console_base;
        al_log_options = options;
        
        setenv("TZ", "UTC+8", 1);
    }

    
#define	LOG_TAG		"TspMsgHandler_Adapter"


#define AL_Log_base(func, level, tag, ...)	AL_Log_base_subsystem(func,	level,	tag,    __VA_ARGS__)
#define LOGI(...)	AL_Log_base(__func__,	LOG_AL_INFO,	LOG_TAG,	__VA_ARGS__)
#define LOGD(...)	AL_Log_base(__func__,	LOG_AL_DEBUG,	LOG_TAG,	__VA_ARGS__)
#define LOGW(...)	AL_Log_base(__func__,	LOG_AL_WARN,	LOG_TAG,	__VA_ARGS__)
#define LOGE(...)	AL_Log_base(__func__,	LOG_AL_ERR,		LOG_TAG,    __VA_ARGS__)



int main()
{
    AL_Log_init(LOG_OPT_DEBUG|LOG_OPT_CONSOLE_OUT|LOG_OPT_TIMESTAMPS|LOG_OPT_FUNC_NAMES);
    int aaa=101;
    LOGI("666666666666666666666   aaa=%d\n",aaa);
   LOGD("666666666666666666666");
   LOGW("777777777777777777777777777777777");
   LOGE("666666666666666666666\n");


}
