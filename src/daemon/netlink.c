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

#include "netlink.h"
#include "genl_def.h"
#include "db.h"

static struct nl_sock *sk = NULL;
static int keymon_mc_group_id = -1;

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
    static struct keymon_event event;

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

    memset(&event, 0, sizeof(struct keymon_event));
    
    event.value = nla_get_u32( attrs[ KEYMON_GENL_ATTR_KEY_VALUE ] );
    event.down  = nla_get_u32( attrs[ KEYMON_GENL_ATTR_KEY_DOWN  ] );
    event.shift = nla_get_u32( attrs[ KEYMON_GENL_ATTR_KEY_SHIFT ] );

    printf("Storing keycode %d, down %d, shiftmask %d\n", event.value, event.down, event.shift);

    keymon_db_store( event );

	return NL_OK;
}

//==========================================================================
//   nl_sock_receive
//==========================================================================
/// Netlink socket message receiving.
void nl_sock_receive()
{
    int rc = 0;

    // Receive messages
    rc = nl_recvmsgs_default( sk );
    if( rc < 0 )
    {
        error( 0, rc, "nl_recvmsgs_default failed" );
        keymon_mc_group_id = -1;
    }
}

//==========================================================================
//   nl_sock_connect
//==========================================================================
/// Netlink socket connecting.
void nl_sock_connect()
{
    int rc = 0;

    // ------------------------------------------------------------
    // Resolve keymon muilticast group 
    // ------------------------------------------------------------
    keymon_mc_group_id = genl_ctrl_resolve_grp( sk, KEYMON_GENL_FAMILY_NAME,
            KEYMON_MC_GROUP_NAME );
    if ( keymon_mc_group_id < 0 )
    {
        error( 0, keymon_mc_group_id, "genl_ctrl_resolve_grp failed" );
        return;
    }

    // ----------------------------
    // Join keymon multicast group
    // ----------------------------
    rc = nl_socket_add_memberships( sk, keymon_mc_group_id, 0 );
    if ( rc < 0 )
    {
        error( 0, rc, "nl_socket_add_membership failed" );
        keymon_mc_group_id = -1;
        return;
    }

    return;
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
