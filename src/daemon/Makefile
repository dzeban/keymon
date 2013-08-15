
CC=gcc

# Get pre-processor, compile and link flags from pkgconfig
# On Fedora it's not 'pkgconfig', it's pkg-config
LIBNL_FLAGS= $(shell pkg-config --cflags --libs libnl-genl-3.0)
LIBDB_FLAGS= -ldb
CFLAGS= $(LIBNL_FLAGS) $(LIBDB_FLAGS) -I../include -Wall -lpthread

all: thread netlink db daemon
	$(CC) $(CFLAGS) -o keymond keymond.o db.o netlink.o thread.o

thread:
	$(CC) $(CFLAGS) -c thread.c

netlink:
	$(CC) $(CFLAGS) -c netlink.c

db:
	$(CC) $(CFLAGS) -c db.c

daemon:
	$(CC) $(CFLAGS) -c keymond.c

clean:
	rm -f *.o keymond 
