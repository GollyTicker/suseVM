CC := /usr/bin/gcc
CCFLAGS := -Wall -std=c99 -DDEBUG_MESSAGES
LDFLAGS :=
SOURCE_FLAG := -D_XOPEN_SOURCE=600 #need because ipc.h require functions from XOPEN_SOURCE library

all: vmappl mmanage

vmappl.o: vmappl.c vmappl.h
	$(CC) $(CCFLAGS) -c -o vmappl.o vmappl.c

vmaccess.o: vmaccess.c vmaccess.h
	$(CC) $(CCFLAGS) -c -o vmaccess.o vmaccess.c

vmappl: vmaccess.o vmappl.o
	$(CC) $(LDFLAGS) -o vmappl vmaccess.o vmappl.o

mmanae.o: mmanage.c mmanage.h
	$(CC) $(CCFLAGE) $(SOURCE_FLAG) -c -o mmanage.o mmanage.h

mmanage: mmanage.o
	$(CC) $(LDFLAGS) -o mmanage mmanage.o

clean:
	rm vmappl *.o
