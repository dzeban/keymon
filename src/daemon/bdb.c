#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "bdb.h"

int main(int argc, const char *argv[])
{
	DB *db_handle;
	int ret;

	ret = db_create( &db_handle, NULL, 0 );
	if( ret )
	{
		printf( "Failed to get DB handle. %s\n", db_strerror( ret ) );
		return EXIT_FAILURE;
	}
	printf( "Got DB handle 0x%p\n", db_handle );

	ret = db_handle->open( db_handle,        // DB handle (you don't say?)
	                       NULL,             // Transaction pointer
	                       DB_FILENAME,      // DB file name on disk
	                       NULL,             // Optional logical db name
	                       DB_ACCESS_METHOD, // Predefined access method
	                       DB_FLAGS,         // Flags to open DB
	                       0);               // File mode (using defaults)
	if( ret )
	{
		printf( "Failed to open DB. %s\n", db_strerror( ret ) );
		return EXIT_FAILURE;
	}
	printf( "Successfully opened DB!\n" );

	//
	// Some code here...
	//

	if( db_handle )
	{
		db_handle->close( db_handle, 0 );
	}
	printf( "Successfully closed DB.\n" );
	
	return EXIT_SUCCESS;
}
