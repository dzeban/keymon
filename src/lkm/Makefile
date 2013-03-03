obj-m := keymon.o

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

module:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
