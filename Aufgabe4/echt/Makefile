
# Große Teile dieses Makefile
# wurden auch aus dem vorgegebenen Beispiel uebernommen.

EXTRA_CFLAGS = -Wall -I$(LDDINC)

# comment this line out to disable debug messages
EXTRA_CFLAGS += -O2 -DDEBUG_MESSAGES

ifneq ($(KERNELRELEASE),)
# call from kernel build system

scull-objs := translate.o

obj-m	:= translate.o

else

# Hinweise zu dieser Ersetztung im Entwurf/Protokoll.
# KERNELDIR ?= /lib/modules/$(shell uname -r)/build
KERNELDIR ?= /lib/modules/3.7.10-1.24-default/build
PWD       := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) LDDINC=$(PWD)/../include modules

endif


clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c *.markers *.order *.symvers .tmp_versions

depend .depend dep:
	$(CC) $(CFLAGS) -M *.c > .depend


ifeq (.depend,$(wildcard .depend))
include .depend
endif
