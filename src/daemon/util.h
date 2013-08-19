#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

#define KM_DBG 0

#define debug(fmt, x...) __debug(fmt"\n", ##x)
static inline void __debug(char *fmt, ...)
{
    va_list arg;

    if(KM_DBG)
    {
        va_start(arg, fmt);
        vprintf(fmt, arg);
        va_end(arg);
    }
}

#endif
