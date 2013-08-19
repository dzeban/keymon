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

#include "db.h"
#include "util.h"

// Handy macro to get value from DBT struct
#define v(x) *(int *)x.data

//==========================================================================
//   rec
//==========================================================================
/// Does DB record stuff
///
/// @param db - DB handle
/// @param event - event to log 
/// @param action - action to do
///
/// Construct DBT structs and makes queries to DB depending on action
///
/// @return 0 on success, DB error otherwise
int rec( DB *db, struct keymon_event event, enum rec_action action )
{
    DBT key, value;
    int k,v;
    int rc = 0;

    debug("In rec. keycode %d, down %d, shiftmask %d", event.value, event.down, event.shift);

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
            rc = db->get(db, NULL, &key, &value, 0);
            debug("Checked keycode %d, rc %d", v(key), rc);
            break;

        case REC_CREATE: // Create new record with initial values from event
            rc = db->put(db, NULL, &key, &value, 0);
            rc = db->get(db, NULL, &key, &value, 0);
            debug("Created record keycode %d, hits %d, rc %d", v(key), v(value), rc);
            break;

        case REC_UPDATE: // Get existing record
            rc = db->get(db, NULL, &key, &value, 0);
            if(rc)
            {
                debug("Updating not-existing keycode!");
                break;
            }

            // Update value with "event.down" value
            (*(int *)value.data) += v;

            // Put updated value back
            rc = db->put(db, NULL, &key, &value, 0);

            debug("Updated keycode %d, hits %d", v(key), v(value));
            break;
    }

    if( db->sync(db,0) )
    {
        syslog(LOG_WARNING, "Failed to sync db\n" );
    }

    return rc;
}

//==========================================================================
//   keymon_db_store
//==========================================================================
/// Store keymon event to DB
///
/// @param event - keymon event to store in DB
///
/// * Check whether event was ever logged
/// * If it's event for new keycode - create new DB
/// * Othrewise update existing record
/// 
void keymon_db_store( struct keymon_event event )
{
    int rc = 0;

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
            syslog(LOG_ERR, "Failed to store key %d: %s\n", event.value, db_strerror(rc));
            break;
    }
    return;
}

void db_cleanup()
{
	if( db )
	{
		db->close( db, 0 );
	}
	syslog(LOG_INFO, "Successfully closed DB.\n" );
}


//==========================================================================
//   db_init
//==========================================================================
/// Initialize DB handle
/// 
/// @return 0 on success, -1 on error
int db_init()
{
	int ret;

	ret = db_create( &db, NULL, 0 );
	if( ret )
	{
		syslog(LOG_ERR, "Failed to get DB handle. %s\n", db_strerror( ret ) );
		return -1;
	}
	syslog(LOG_INFO, "Got DB handle 0x%p\n", db );

	ret = db->open( db,               // DB handle (you don't say?)
	                NULL,             // Transaction pointer
	                DB_FILENAME,      // DB file name on disk
	                NULL,             // Optional logical db name
	                DB_ACCESS_METHOD, // Predefined access method
	                DB_FLAGS,         // Flags to open DB
	                0);               // File mode (using defaults)
	if( ret )
	{
		syslog(LOG_ERR, "Failed to open DB. %s\n", db_strerror( ret ) );
		return -1;
	}
	syslog(LOG_INFO, "Successfully opened DB %s!\n", DB_FILENAME );

	return 0;
}
