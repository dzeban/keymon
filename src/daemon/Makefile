
CC=gcc

# Get pre-processor, compile and link flags from pkgconfig
# On Fedora it's not 'pkgconfig', it's pkg-config
LIBNL_FLAGS= $(shell pkg-config --cflags --libs libnl-genl-3.0)
LIBDB_FLAGS= -ldb
CFLAGS= $(LIBNL_FLAGS) $(LIBDB_FLAGS) -Wall -lpthread

all: netlink bdb daemon
	$(CC) $(CFLAGS) -o keymond keymond.o bdb.o netlink.o

netlink:
	$(CC) $(CFLAGS) -c netlink.c

bdb:
	$(CC) $(CFLAGS) -c bdb.c

daemon:
	$(CC) $(CFLAGS) -c keymond.c

clean:
	rm -f *.o keymond 
