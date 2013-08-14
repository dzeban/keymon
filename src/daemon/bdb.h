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
#ifndef BDB_H
#define BDB_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <db.h>

#define DB_FILENAME      "keymond.db"
#define DB_ACCESS_METHOD DB_BTREE
#define DB_FLAGS         DB_CREATE

DB *db;

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

int db_init();
void keymon_db_store( struct keymon_event event );

#endif // BDB_H
