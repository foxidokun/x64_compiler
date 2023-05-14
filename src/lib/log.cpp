#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "log.h"

//----------------------------------------------------------------------------------------------------------------------
// Consts
//----------------------------------------------------------------------------------------------------------------------

#define R       "\033[91m"
#define G       "\033[92m"
#define Cyan    "\033[96m"
#define Y       "\033[93m"
#define D       "\033[39m"
#define Bold    "\033[1m"
#define Plain   "\033[0m"

const size_t __TIME_BUF_SIZE = 10;

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

static inline void current_time (char *buf, size_t buf_size);

//----------------------------------------------------------------------------------------------------------------------
// Static variables
//----------------------------------------------------------------------------------------------------------------------
log LOG_LEVEL = log::DEBUG;
FILE *LOG_OUT_STREAM = stdout;

//----------------------------------------------------------------------------------------------------------------------
// Public
//----------------------------------------------------------------------------------------------------------------------

void _log (log lvl, const char *fmt, const char *file, unsigned int line...)
{
    va_list args;
    va_start (args, line);

    if (lvl >= LOG_LEVEL)
    {
        char time_buf[__TIME_BUF_SIZE] = "";
        current_time (time_buf, __TIME_BUF_SIZE);
        fprintf (LOG_OUT_STREAM, "%s ", time_buf);

        if      (lvl == log::DEBUG) { fprintf (LOG_OUT_STREAM, "DEBUG"); }
        else if (lvl == log::INFO)  { fprintf (LOG_OUT_STREAM, Cyan "INFO " D); }
        else if (lvl == log::WARN)  { fprintf (LOG_OUT_STREAM, Y "WARN " D); fflush (LOG_OUT_STREAM); }
        else if (lvl == log::ERROR) { fprintf (LOG_OUT_STREAM, R "ERROR" D); fflush (LOG_OUT_STREAM); }

        fprintf (LOG_OUT_STREAM, " [%s:%u] ", file, line);
        vfprintf (LOG_OUT_STREAM, fmt, args);
        fputc   ('\n', LOG_OUT_STREAM);
    }

    va_end (args);
}

//----------------------------------------------------------------------------------------------------------------------
// Log managment
//----------------------------------------------------------------------------------------------------------------------

void set_log_level (log level)
{
    LOG_LEVEL = level;
}

void set_log_stream (FILE *stream)
{
    assert (stream);

    LOG_OUT_STREAM = stream;
}

FILE *get_log_stream ()
{
    return LOG_OUT_STREAM;
}

//----------------------------------------------------------------------------------------------------------------------
// Static
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief      Write current time in HH:MM:SS format to given buffer
 *
 * @param[out] buf       Output buffer
 * @param[in]  buf_size  Buffer size
 */
static inline void current_time (char *buf, size_t buf_size)
{
    assert (buf != nullptr && "pointer can't be null");
    assert (buf_size >= 9 && "Small buffer");

    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    strftime (buf, buf_size, "%H:%M:%S", timeinfo);
}