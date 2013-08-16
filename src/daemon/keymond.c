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

#include "keymond.h"

//=========================================
//      main
//=========================================
/// Keymond entry point.
///
/// * Does DB initialization.
/// * Does netlink socket initialization.
/// * Does threads configuration (run automatically).
/// * Waits for threads to stop.
int main(int argc, const char *argv[])
{
    openlog(SYSLOG_IDENT, SYSLOG_OPT, SYSLOG_FACILITY);

    if( db_init() < 0 )
    {
        syslog(LOG_ERR, "db_init" );
        return EXIT_FAILURE;
    }

	if( nl_sock_init() < 0 )
	{
		syslog(LOG_ERR, "nl_sock_init" );
		return EXIT_FAILURE;
	}

    if( thread_init() < 0 )
    {
        syslog(LOG_ERR, "thread_init" );
        return EXIT_FAILURE;
    }
    
    pthread_exit( NULL );
	return EXIT_SUCCESS;
}
