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

//==========================================================================
//   receiver
//==========================================================================
/// Netlink socket message parsing callback
/// 
/// @param msg - received message
/// @param arg - not used
/// 
/// Parse and print received keymon message based upon attributes definition
/// ( shared with kernel module ).
/// 
/// @return NL_OK on success, NL_SKIP on error.
int receiver(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = NULL;
	struct nlattr *attrs[ KEYMON_GENL_ATTR_MAX + 1 ];
    static char buf[20];
    static char hex[6];
    int i = 0;

	nlh = nlmsg_hdr( msg );
	if( !nlh )
	{
		printf( "Failed to get message header\n" );
		return NL_SKIP;
	}

	if( genlmsg_parse(nlh, 0, attrs, KEYMON_GENL_ATTR_MAX, keymon_nla_policy ) < 0)
	{
		printf( "Failed to parse netlink message\n" );
		return NL_SKIP;
	}

    memset(buf, 0, 12);
    for( i = KEYMON_GENL_ATTR_MIN; i <= KEYMON_GENL_ATTR_MAX; i++ )
    {
        if( attrs[ i ] )
        {
            // Print 2 hex digits into right place in buf
            snprintf( hex, 6, "%c:%02X ", keymon_attrs_names[i-1], nla_get_u32( attrs[i] ) );
		    strncat( buf, hex, 6 );
        }
    }

    printf("%s\n", buf);

	return NL_OK;
}

//==========================================================================
//   nl_sock_cleanup
//==========================================================================
/// Netlink socket destroying.
void nl_sock_cleanup()
{
	if( sk )
	{
		nl_socket_free( sk );
	}

    return;
}

//==========================================================================
//   nl_sock_init
//==========================================================================
/// Initialize netlink socket
///
/// * Allocate socket
/// * Set callback for parsing received messages
/// * Connect to generic netlink bug
/// * Switch socket to non-blocking mode
/// 
/// @return 0 on success, -1 on error
int nl_sock_init()
{
	int rc = 0;
	struct nl_cache *genl_cache = NULL;

	// ----------------------
	// Allocate a new socket
	// ----------------------
	sk = nl_socket_alloc();
	if( !sk )
	{
		error( 0, 0, "nl_socket_alloc failed" );
		rc = -1;
		goto fail;
	}
	printf( "Netlink socket allocated (%p)\n", sk );

	// Disable sequence number checking
	nl_socket_disable_seq_check( sk );

	// ------------------------------
	// Set socket callbacks
	// ------------------------------
	
	// Entry callback for valid incoming messages
	rc = nl_socket_modify_cb( sk, NL_CB_VALID, NL_CB_CUSTOM, receiver, NULL );
	if( rc < 0 )
	{
		error( 0, rc, "nl_cb_set failed");
		rc = -1;
		goto fail;
	}

	// -------------------------------
	// Connect to Generic Netlink bus
	// -------------------------------
	rc = genl_connect( sk );
	if( rc < 0 )
	{
		error( 0, rc, "genl_connect failed" );
		rc = -1;
		goto fail;
	}
	printf( "Netlink socket connected \n" );

	// ------------------------------------- 
	// Allocate libnl generic netlink cache
	// ------------------------------------- 
	rc = genl_ctrl_alloc_cache( sk, &genl_cache );
	if( rc < 0 )
	{
		error( 0, rc, "genl_ctrl_alloc_cache failed" );
		rc = -1;
		goto fail;
	}
	printf( "genl_cache allocated (%p)\n", genl_cache );
	
	// -----------------------------------
    // Switch socket to non-blocking mode
    // -----------------------------------
	if( nl_socket_set_nonblocking( sk ) < 0 )
	{
		error( 0, 0, "nl_socket_set_nonblocking failed" );
		rc = -1;
		goto fail;
	}
	printf( "Netlink socket is set to non-blocking mode\n" );

	return rc;

fail:
	if( genl_cache )
	{
		nl_cache_free( genl_cache );
	}

    nl_sock_cleanup();

	return rc;
}

//==========================================================================
//   keymon_connect
//==========================================================================
/// Connector thread work routine
void *keymon_connect()
{
	int rc = 0;

    while(1)
    {
        pthread_mutex_lock( &sk_mutex );

        // ------------------------------------------------------------
        // Resolve keymon muilticast group 
        // ------------------------------------------------------------
        keymon_mc_group_id = genl_ctrl_resolve_grp( sk, KEYMON_GENL_FAMILY_NAME,
                                                           KEYMON_MC_GROUP_NAME );
        if ( keymon_mc_group_id < 0 )
        {
            error( 0, keymon_mc_group_id, "genl_ctrl_resolve_grp failed" );
            goto sleep_cont;
        }

        // ----------------------------
        // Join keymon multicast group
        // ----------------------------
        rc = nl_socket_add_memberships( sk, keymon_mc_group_id, 0 );
        if ( rc < 0 )
        {
            error( 0, rc, "nl_socket_add_membership failed" );
            keymon_mc_group_id = -1;
            goto sleep_cont;
        }

sleep_cont:
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
    int rc = 0;
    while(1)
    {
        pthread_mutex_lock( &sk_mutex );
        
        // Receive messages
        rc = nl_recvmsgs_default( sk );
        if( rc < 0 )
        {
            error( 0, rc, "nl_recvmsgs_default failed" );
            keymon_mc_group_id = -1;
        }
        pthread_mutex_unlock( &sk_mutex );
    }
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
    pthread_attr_t attr;

    rc = pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );
    
    pthread_mutex_init( &sk_mutex, NULL );

    if( rc < 0 )
    {
        perror("pthread_attr_init");
        goto fail;
    }

    rc = pthread_create( &connector_thread, &attr, keymon_connect, NULL );
    if( rc )
    {
        perror("pthread_create");
        goto fail;
    }

    rc = pthread_create( &receiver_thread, &attr, keymon_receive, NULL );
    if( rc )
    {
        perror("pthread_create");
        goto fail;
    }

    rc = pthread_attr_destroy( &attr );
    if( rc )
    {
        perror("pthread_attr_destroy");
        goto fail;
    }
    return 0;

fail:
    nl_sock_cleanup();
    return -1;
}

//=========================================
//      main
//=========================================
/// Keymond entry point.
///
/// * Does netlink socket initialization.
/// * Does threads initialization.
/// * Waits for threads to stop.
int main(int argc, const char *argv[])
{
	if( nl_sock_init() < 0 )
	{
		perror( "nl_sock_init");
		return EXIT_FAILURE;
	}

    if( thread_init() < 0 )
    {
        perror( "thread_init" );
        return EXIT_FAILURE;
    }
    
    pthread_exit( NULL );
	return EXIT_SUCCESS;
}
