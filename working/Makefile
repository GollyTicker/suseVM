CC=/usr/bin/gcc
CFLAGS=-Wall
LDFLAGS=-lrt

CFLAGS += -DLFU -pthread
#CFLAGS += -DFIFO
#CFLAGS += -DCLOCK
#CFLAGS += -DDEBUG_MESSAGES

all: mmanage vmappl

mmanage: mmanage.c mmanage.h
	$(CC) $(CFLAGS) -o mmanage mmanage.c $(LDFLAGS)

vmappl: vmappl.c vmappl.h vmaccess.c vmaccess.h
	$(CC) $(CFLAGS) -o vmappl vmappl.c vmaccess.c $(LDFLAGS)

clean: 
	rm -f mmanage vmappl
