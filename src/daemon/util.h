/*
 * Keymon - keyboard monitor and statistic system.
 * See details on http://reduct.ru/~avd/keymon
 *
 * Copyright (C) 2013  Alex Dzyoba <avd@reduct.ru>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>
#include <signal.h>

sig_atomic_t KM_DBG;

#define debug(fmt, x...) if( KM_DBG ){ __debug(fmt"\n", ##x); }
static inline void __debug(char *fmt, ...)
{
    va_list arg;

    va_start(arg, fmt);
    vsyslog(LOG_DEBUG, fmt, arg);
    va_end(arg);
}

#endif
