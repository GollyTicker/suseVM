DEBUG = y
#Debugging Flag hinzufügen oder nicht
ifeq ($(DEBUG),y)
	DEBFLAGS = -O2 -g -DSCULL_DEBUG
else
	DEBFLAGS = -O2
endif

EXTRA_CFLAGS += $(DEBFLAGS)
EXTRA_CFLAGS += -I$(LDDINC)
EXTRA_CFLAGS += -DDEBUG_MESSAGES

ifneq ($(KERNELRELEASE),)
# call from kernel build system

scull-objs := translate.o

obj-m	:= translate.o

else

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
