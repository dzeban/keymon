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

int construct_attrs(struct nl_msg *msg)
{
	NLA_PUT_U32(msg, KEYMON_GENL_ATTR_PID, 182);

	return 0;

nla_put_failure: // Reserved label used by NLA_PUT_* macros
	return -EMSGSIZE;
}

int main(int argc, const char *argv[])
{
	struct nl_sock *sk = NULL;
	struct nl_cache *genl_cache = NULL;
	struct genl_family *keymon_genl_family = NULL;
	struct nl_msg *msg = NULL;
	void *user_hdr = NULL;
	int keymon_family_id = 0;
	int ret = EXIT_SUCCESS;

	// ----------------------
	// Allocate a new socket
	// ----------------------
	sk = nl_socket_alloc();
	if( !sk )
	{
		perror( "nl_socket_alloc" );
		ret = EXIT_FAILURE;
		goto fail;
	}
	printf( "Netlink socket allocated (%p)\n", sk );

	// -------------------------------
	// Connect to Generic Netlink bus
	// -------------------------------
	ret = genl_connect( sk );
	if( ret < 0 )
	{
		perror( "genl_connect" );
		printf( "ret is %d\n", ret );
		ret = EXIT_FAILURE;
		goto fail;
	}
	printf( "Netlink socket connected \n" );

	// ------------------------------------- 
	// Allocate libnl generic netlink cache
	// ------------------------------------- 
	ret = genl_ctrl_alloc_cache( sk, &genl_cache );
	if( ret < 0 )
	{
		perror( "genl_ctrl_alloc_cache" );
		printf( "ret is %d\n", ret );
		ret = EXIT_FAILURE;
		goto fail;
	}
	printf( "genl_cache allocated (%p)\n", genl_cache );

	// ------------------------------------------------
	// Find Keymon kernel space generic family by name
	// ------------------------------------------------
	keymon_genl_family = genl_ctrl_search_by_name( genl_cache,
	                                               KEYMON_GENL_FAMILY_NAME );

	if( !keymon_genl_family )
	{
		perror( "genl_ctrl_search_by_name" );
		ret = EXIT_FAILURE;
		goto fail;
	}
	printf( "keymon_genl_family found (%p)\n", keymon_genl_family );

	// ------------------------------------------------------------
	// Resolve generic netlink family id (needed to send messages)
	// ------------------------------------------------------------
	keymon_family_id = genl_ctrl_resolve( sk, KEYMON_GENL_FAMILY_NAME );
	if ( keymon_family_id < 0 )
	{
		perror( "genl_ctrl_resolve" );
		ret = EXIT_FAILURE;
		goto fail;
	}

	printf( "keymon_family_id is %d\n", keymon_family_id );

	// ----------------------
	// Message construction
	// ----------------------
	msg = nlmsg_alloc();
	if( !msg )
	{
		perror("nlmsg_alloc");
		ret = EXIT_FAILURE;
		goto fail;
	}

	// Adds generic netlink header to netlink message
	user_hdr = genlmsg_put( msg, 
	                        NL_AUTO_PORT, 
	                        NL_AUTO_SEQ, 
	                        keymon_family_id, 
	                        0, // User header length
	                        0, // Flags
	                        KEYMON_GENL_CMD_REGISTER, 
	                        KEYMON_GENL_VERSION );
	
	printf( "User header after genlmsg_put is %p\n", user_hdr );
	
	// Add attributed value
	ret = construct_attrs( msg );
	if( ret < 0 )
	{
		perror( "construct_attrs" );
		ret = EXIT_FAILURE;
		goto fail;
	}

	// ---------------
	// SEND MESSAGE
	// ---------------
	ret = nl_send_auto( sk, msg );
	if( ret < 0 )
	{
		perror( "nl_send_auto" );
		ret = EXIT_FAILURE;
		goto fail;
	}

	printf( "MESSAGE SENT (%d bytes)!\n", ret );

fail:
	
	if( msg )
	{
		nlmsg_free( msg );
	}

	if( keymon_genl_family )
	{
		genl_family_put( keymon_genl_family );
	}

	if( genl_cache )
	{
		nl_cache_free( genl_cache );
	}

	if( sk )
	{
		nl_socket_free( sk );
	}

	return ret;
	
}
