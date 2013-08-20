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
#include "util.h"


//=========================================
//   signal_handler
//=========================================
///
/// SIGUSR1: switch KM_DBG to enable/disable extensive debug logging
/// 
void signal_handler(int signo)
{
    KM_DBG = ~KM_DBG;
    printf("Caught signal %d. KM_DBG %d\n", signo, KM_DBG);
}

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
    int rc = 0;

    // -----------------------------
    // Logging
    // -----------------------------
    openlog(SYSLOG_IDENT, SYSLOG_OPT, SYSLOG_FACILITY);

    // -----------------------------
    // Signal handling
    // -----------------------------
    struct sigaction sa_act;
    sa_act.sa_handler = signal_handler;

    rc = sigaction(SIGUSR1, &sa_act, NULL);
    if( rc )
    {
        syslog(LOG_WARNING, "sigaction");
        return EXIT_FAILURE;
    }

    // -----------------------------
    // Database initiatlization
    // -----------------------------
    if( db_init() < 0 )
    {
        syslog(LOG_ERR, "db_init" );
        return EXIT_FAILURE;
    }

    // -----------------------------
    // Netlink initialization
    // -----------------------------
	if( nl_sock_init() < 0 )
	{
		syslog(LOG_ERR, "nl_sock_init" );
		return EXIT_FAILURE;
	}

    // -----------------------------
    // Daemonize!
    // -----------------------------
    // We do this after all initialization to allow opening local DB file, but 
    // just before threading because threads are not inherited after 
    // daemonization fork.
    daemon(0,0);

    // -----------------------------
    // Thread initialization
    // -----------------------------
    if( thread_init() < 0 )
    {
        syslog(LOG_ERR, "thread_init" );
        return EXIT_FAILURE;
    }
    
    // -----------------------------
    // Wait for threads to stop
    // -----------------------------
    pthread_exit( NULL );
	return EXIT_SUCCESS;
}
