
#include "netlink.h"

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
