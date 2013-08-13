#!/usr/bin/env python

# Assumed to work with python3

import bsddb3
import struct
import sys

dbfile = sys.argv[1]

db = bsddb3.btopen(dbfile,'r')

for k,v in db.items():
    key = struct.unpack('=L',k)[0]
    value = struct.unpack('=L',v)[0]

    print(repr(key) + ":" + repr(value))

