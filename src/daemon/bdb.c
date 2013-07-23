#include "bdb.h"

struct keymon_event events[] = {
    {22,1,0},
    {22,2,0},
    {22,2,0},
    {22,2,0},
    {22,2,0},
    {22,2,0},
    {22,2,0},
    {15,1,0},
    {63,1,0},
    {11,1,0},
    {63,1,0},
    { 8,1,0},
    {55,1,2},
    {55,1,4},
    {55,1,4},
    {55,1,4},
};

int rec( DB *db, struct keymon_event event, enum rec_action action )
{
    DBT key, value;
    struct keymon_record rec;
    int rc = 0;

    // Initialize DB things
    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    key.data = &event.value;
    key.size = sizeof(int);

    value.data = &rec;
    value.ulen = sizeof(struct keymon_record);
    value.flags = DB_DBT_USERMEM; // We provide own buffer to get/put data

    switch(action)
    {
        case REC_CHECK:
            // Simply get record by event value
            rc = db->get(db, NULL, &key, &value, 0);
            break;

        case REC_CREATE:
            // Create new record with initial values from event
            rec.keycode = event.value;
            rec.hits    = event.down;
            rc = db->put(db, NULL, &key, &value, 0);
            break;

        case REC_UPDATE:
            // Get existing record
            rc = db->get(db, NULL, &key, &value, 0);
            if(rc)
            {
                printf("This is really strange if we are here...\n");
                break;
            }
            printf("Got Keycode %d. Hits %d\n", rec.keycode, rec.hits);

            // Update value pointed by DBT value. 
            // This is why we need DB_DBT_USERMEM flag.
            rec.hits++;

            // Put updated value back
            rc = db->put(db, NULL, &key, &value, 0);
            break;
    }

    return rc;
}

void test_store(DB *db)
{
    int i = 0;
    int rc = 0;
    int nevents = sizeof(events) / sizeof(struct keymon_event);

    for(i = 0; i < nevents; i++)
    {
        rc = rec(db, events[i], REC_CHECK);
        switch(rc)
        {
            case DB_NOTFOUND:
                rec(db, events[i], REC_CREATE);
                break;
            case 0:
                rec(db, events[i], REC_UPDATE);
                break;
            default:
                printf("Key %d: %s\n", events[i].value, db_strerror(rc));
                break;
        }
    }
    return;
}

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

    test_store(db_handle);

	if( db_handle )
	{
		db_handle->close( db_handle, 0 );
	}
	printf( "Successfully closed DB.\n" );
	
	return EXIT_SUCCESS;
}
