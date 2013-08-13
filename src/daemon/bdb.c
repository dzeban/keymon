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

#include "bdb.h"

#define v(x) *(int *)x.data

int rec( DB *db, struct keymon_event event, enum rec_action action )
{
    DBT key, value;
    int k,v;
    int rc = 0;

    printf("In rec. keycode %d, down %d, shiftmask %d\n", event.value, event.down, event.shift);

    memset(&key,   0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    k = event.value;
    v = event.down;

    key.data = &k;
    key.size = sizeof(int);

    value.data = &v;
    value.size = sizeof(int);

    switch(action)
    {
        case REC_CHECK: // Simply get record by event value

            printf(":: Checking keycode %d\n", v(key));

            rc = db->get(db, NULL, &key, &value, 0);
            if( rc != DB_NOTFOUND )
            {
                printf(":: Existing keycode %d, hits %d\n", v(key), v(value));
            }
            break;

        case REC_CREATE: // Create new record with initial values from event

            printf("# Creating record for keycode %d, hits %d, rc %d\n", v(key), v(value), rc);
            rc = db->put(db, NULL, &key, &value, 0);
            rc = db->get(db, NULL, &key, &value, 0);
            printf("# Created record keycode %d, hits %d, rc %d\n", v(key), v(value), rc);

            break;

        case REC_UPDATE: // Get existing record

            printf("=> Updating keycode %d\n", v(key));
            rc = db->get(db, NULL, &key, &value, 0);
            if(rc)
            {
                printf("This is really strange if we are here...\n");
                break;
            }
            printf("=> Got Keycode %d. Hits %d\n", v(key), v(value));

            // Update value pointed by DBT value. 
            // This is why we need DB_DBT_USERMEM flag.
            (*(int *)value.data)++;

            // Put updated value back
            rc = db->put(db, NULL, &key, &value, 0);

            rc = db->get(db, NULL, &key, &value, 0);
            printf("=> Updated keycode %d, hits %d\n", v(key), v(value));
            break;
    }

    if( db->sync(db,0) )
    {
        printf( "Failed to sync db\n" );
    }

    printf("\n");
    return rc;
}

void keymon_db_store( struct keymon_event event )
{
    int rc = 0;

    printf("-----------------------\n");

    // First we make test query to check if we've already logged such event
    rc = rec(db, event, REC_CHECK);

    switch(rc)
    {
        // If it's new keycode
        case DB_NOTFOUND:
            rec(db, event, REC_CREATE);
            break;
        // If it's existing code
        case 0:
            rec(db, event, REC_UPDATE);
            break;
        default:
            printf("Key %d: %s\n", event.value, db_strerror(rc));
            break;
    }
    printf("-----------------------\n");
    return;
}

void db_cleanup()
{
	if( db )
	{
		db->close( db, 0 );
	}
	printf( "Successfully closed DB.\n" );
}

int db_init()
{
	int ret;

	ret = db_create( &db, NULL, 0 );
	if( ret )
	{
		printf( "Failed to get DB handle. %s\n", db_strerror( ret ) );
		return -1;
	}
	printf( "Got DB handle 0x%p\n", db );

	ret = db->open( db,               // DB handle (you don't say?)
	                NULL,             // Transaction pointer
	                DB_FILENAME,      // DB file name on disk
	                NULL,             // Optional logical db name
	                DB_ACCESS_METHOD, // Predefined access method
	                DB_FLAGS,         // Flags to open DB
	                0);               // File mode (using defaults)
	if( ret )
	{
		printf( "Failed to open DB. %s\n", db_strerror( ret ) );
		return -1;
	}
	printf( "Successfully opened DB %s!\n", DB_FILENAME );

	return 0;
}
