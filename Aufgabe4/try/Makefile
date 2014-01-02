obj-m += hello.o
 
all:
	make -C /lib/modules/3.7.10-1.24-default/build M=$(PWD) modules
	#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
 
clean:
	make -C /lib/modules/3.7.10-1.24-default/build M=$(PWD) clean
	#make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
