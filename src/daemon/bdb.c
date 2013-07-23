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

int check_record( DB *db, struct keymon_event event )
{
    DBT key, value;
    struct keymon_record rec;
    int rc = 0;

    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    key.data = &event.value;
    key.size = sizeof(int);

    value.data = &rec;
    value.size = sizeof(struct keymon_record);

    rc = db->get(db, NULL, &key, &value, 0);
    return rc;
}

int create_record( DB *db, struct keymon_event event )
{
    DBT key, value;
    struct keymon_record rec;
    int rc = 0;

    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    rec.keycode = event.value;
    rec.hits    = event.down;

    key.data = &event.value;
    key.size = sizeof(int);

    value.data = &rec;
    value.size = sizeof(struct keymon_record);

    rc = db->put(db, NULL, &key, &value, 0);
    return rc;
}

int update_record( DB *db, struct keymon_event event )
{
    DBT key, value;
    struct keymon_record rec;
    int rc = 0;

    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    key.data = &event.value;
    key.size = sizeof(int);

    value.data = &rec;
    value.ulen = sizeof(struct keymon_record);
    value.flags = DB_DBT_USERMEM;

    rc = db->get(db, NULL, &key, &value, 0);
    if(rc)
    {
        return rc;
    }

    printf("Got Keycode %d. Hits %d\n", rec.keycode, rec.hits);

    rec.hits++;

    rc = db->put(db, NULL, &key, &value, 0);

    return rc;
}

void test_store(DB *db)
{
    int i = 0;
    int rc = 0;
    int nevents = sizeof(events) / sizeof(struct keymon_event);

    for(i = 0; i < nevents; i++)
    {
        rc = check_record(db, events[i]);
        switch(rc)
        {
            case DB_NOTFOUND:
                create_record(db, events[i]);
                break;
            case 0:
                update_record(db, events[i]);
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
