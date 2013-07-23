#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <db.h>

#define DB_FILENAME      "keymond.db"
#define DB_ACCESS_METHOD DB_BTREE
#define DB_FLAGS         DB_CREATE

struct keymon_event {
    int value;
    int down;
    int shift;
};

struct keymon_record {
    int keycode;
    int hits;
};

enum rec_action
{
    REC_CHECK,
    REC_CREATE,
    REC_UPDATE
};
