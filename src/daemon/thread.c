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

#include "thread.h"

//==========================================================================
//   keymon_connect
//==========================================================================
/// Connector thread work routine
void *keymon_connect()
{
    while(1)
    {
        pthread_mutex_lock( &sk_mutex );
        nl_sock_connect();
        pthread_mutex_unlock( &sk_mutex );

        sleep(5);
    }

    pthread_exit( NULL );
}

//==========================================================================
//   keymon_receive
//==========================================================================
/// Receiver thread work routine
void *keymon_receive()
{
    while(1)
    {
        pthread_mutex_lock( &sk_mutex );
        nl_sock_receive();
        pthread_mutex_unlock( &sk_mutex );
    }
    pthread_exit( NULL );
}

//==========================================================================
//   thread_init
//==========================================================================
/// Keymond threads initialization.
///
/// Creates netlink connector thread.
/// Creates message receiver thread.
/// In case of any failure destroys netlink socket and return to main error
/// code.
///
/// @return 0 on succes and -1 on error.
int thread_init()
{
    int rc = 0;
    sigset_t sigset, oldset;
    pthread_attr_t attr;

    rc = pthread_attr_init( &attr );
    if( rc < 0 )
    {
        syslog(LOG_ERR, "pthread_attr_init failed, rc %d\n", rc);
        goto fail;
    }
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
    
    pthread_mutex_init( &sk_mutex, NULL );

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    rc = pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
    if( rc )
    {
        syslog(LOG_ERR, "pthread_sigmask failed, rc %d\n", rc);
        goto fail;
    }

    rc = pthread_create( &connector_thread, &attr, keymon_connect, NULL );
    if( rc )
    {
        syslog(LOG_ERR, "pthread_create for connector thread failed, rc %d\n", rc);
        goto fail;
    }

    rc = pthread_create( &receiver_thread, &attr, keymon_receive, NULL );
    if( rc )
    {
        syslog(LOG_ERR, "pthread_create for receiver thread failed, rc %d\n", rc);
        goto fail;
    }

    rc = pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    if( rc )
    {
        syslog(LOG_ERR, "pthread_sigmask failed, rc %d\n", rc);
        goto fail;
    }

    rc = pthread_attr_destroy( &attr );
    if( rc )
    {
        syslog(LOG_ERR, "pthread_attr_destroy failed, rc %d\n", rc);
        goto fail;
    }
    pthread_join(connector_thread, NULL);
    pthread_join(receiver_thread, NULL);

    return 0;

fail:
    nl_sock_cleanup();
    return -1;
}
