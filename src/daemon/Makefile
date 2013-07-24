
CC=gcc

# Get pre-processor, compile and link flags from pkgconfig
# On Fedora it's not 'pkgconfig', it's pkg-config
LIBNL_FLAGS= $(shell pkg-config --cflags --libs libnl-genl-3.0)
LIBDB_FLAGS= -ldb
CFLAGS= $(LIBNL_FLAGS) $(LIBDB_FLAGS) -Wall -lpthread

daemon:
	$(CC) $(CFLAGS) -o keymond keymond.c bdb.c

clean:
	rm -f keymond bdb
