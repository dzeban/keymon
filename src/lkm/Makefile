obj-m := keymon.o

EXTRA_CFLAGS += -I$(PWD)/../include
KDIR := /lib/modules/$(shell uname -r)/build

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
