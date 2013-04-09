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

int receiver(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = NULL;
	struct nlattr *attrs[ KEYMON_GENL_ATTR_MAX + 1 ];
	int data_len = 0;
	char *data = NULL;
	int i = 0;

	printf( "Received message\n" );

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

	if( attrs[ KEYMON_GENL_ATTR_KEY_VALUE ] )
	{
		printf( "Key value %02X\n", nla_get_u32(attrs[ KEYMON_GENL_ATTR_KEY_VALUE ]) );
	}

	if( attrs[ KEYMON_GENL_ATTR_KEY_DOWN ] )
	{
		printf( "Key down %02X\n", nla_get_u8(attrs[ KEYMON_GENL_ATTR_KEY_DOWN ]) );
	}

	if( attrs[ KEYMON_GENL_ATTR_KEY_SHIFT ] )
	{
		printf( "Key shift %02X\n", nla_get_u8(attrs[ KEYMON_GENL_ATTR_KEY_SHIFT ]) );
	}

	if( attrs[ KEYMON_GENL_ATTR_KEY_LEDSTATE ] )
	{
		printf( "Key ledstate %02X\n", nla_get_u8(attrs[ KEYMON_GENL_ATTR_KEY_LEDSTATE ]) );
	}

	return NL_OK;
}

int main(int argc, const char *argv[])
{
	struct nl_sock *sk = NULL;
	struct nl_cache *genl_cache = NULL;
	int keymon_mc_group_id = 0;
	int rc = EXIT_SUCCESS;

	// ----------------------
	// Allocate a new socket
	// ----------------------
	sk = nl_socket_alloc();
	if( !sk )
	{
		perror( "nl_socket_alloc" );
		rc = EXIT_FAILURE;
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

	// -------------------------------
	// Connect to Generic Netlink bus
	// -------------------------------
	rc = genl_connect( sk );
	if( rc < 0 )
	{
		perror( "genl_connect" );
		printf( "rc is %d\n", rc );
		rc = EXIT_FAILURE;
		goto fail;
	}
	printf( "Netlink socket connected \n" );

	// ------------------------------------- 
	// Allocate libnl generic netlink cache
	// ------------------------------------- 
	rc = genl_ctrl_alloc_cache( sk, &genl_cache );
	if( rc < 0 )
	{
		perror( "genl_ctrl_alloc_cache" );
		printf( "rc is %d\n", rc );
		rc = EXIT_FAILURE;
		goto fail;
	}
	printf( "genl_cache allocated (%p)\n", genl_cache );

	// ------------------------------------------------------------
	// Resolve keymon muilticast group 
	// ------------------------------------------------------------
	keymon_mc_group_id = genl_ctrl_resolve_grp( sk, KEYMON_GENL_FAMILY_NAME,
	                                                   KEYMON_MC_GROUP_NAME );
	if ( keymon_mc_group_id < 0 )
	{
		perror( "genl_ctrl_resolve_grp" );
		rc = EXIT_FAILURE;
		goto fail;
	}
	printf( "keymon_mc_group_id is %d\n", keymon_mc_group_id );

	// ----------------------------
	// Join keymon multicast group
	// ----------------------------
	rc = nl_socket_add_memberships( sk, keymon_mc_group_id, 0 );
	if ( rc < 0 )
	{
		perror( "nl_socket_add_membership" );
		rc = EXIT_FAILURE;
		goto fail;
	}

	// ------------------------------------------------------------------------
	// Start receiving messages. The function nl_recvmsgs_default() will block
	// until one or more netlink messages (notification) are received which
	// will be passed on to receiver().
	// ------------------------------------------------------------------------
	while (1)
	{
		nl_recvmsgs_default(sk);
	}

fail:
	if( genl_cache )
	{
		nl_cache_free( genl_cache );
	}

	if( sk )
	{
		nl_socket_free( sk );
	}

	return rc;
}
