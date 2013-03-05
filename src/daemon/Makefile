
CC=gcc

# Get pre-processor, compile and link flags from pkgconfig
# On Fedora it's not 'pkgconfig', it's pkg-config
LIBNL_FLAGS= $(shell pkg-config --cflags --libs libnl-genl-3.0)
CFLAGS= $(LIBNL_FLAGS) -Wall

daemon:
	@echo $(LIBNL_FLAGS)
	$(CC) $(CFLAGS) keymond.c -o keymond

clean:
	rm -f keymond
