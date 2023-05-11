#ifndef LOG_H
#define LOG_H

#include <assert.h>
#include <stdio.h>
#include <time.h>

//----------------------------------------------------------------------------------------------------------------------

enum class log
{
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
};

//----------------------------------------------------------------------------------------------------------------------
// Log macro
//----------------------------------------------------------------------------------------------------------------------

/**
 * @brief      Write to log stream time, file&line and your formatted message
 *
 * @param      lvl   Log level
 * @param[in]  fmt   Format string
 * @param[in]  file  File where log is called  
 * @param      line  Line where log is called
 * @param      ...   printf parameters (format string & it's parameters)
 */
#ifndef DISABLE_LOGS

void _log (enum log lvl, const char *fmt, const char *file, unsigned int line, ...);

#define log(lvl, fmt, ...)                               \
{                                                        \
    _log (log::lvl, fmt, __FILE__, __LINE__, ##__VA_ARGS__);  \
}                                                       

#else

#define log(lvl, ...) {;}

#endif
/**
 * @brief      Sets the log level.
 *
 * @param[in]  level  The level
 */

//----------------------------------------------------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------------------------------------------------

void set_log_level (enum log level);

/**
 * @brief      Sets the output log stream.
 *
 * @param      stream  Stream
 */
void set_log_stream (FILE *stream);

FILE *get_log_stream ();

#endif //LOG_H