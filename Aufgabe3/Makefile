CC := /usr/bin/gcc
CCFLAGS := -Wall -std=c99 
LDFLAGS :=
SOURCE_FLAG := -D_XOPEN_SOURCE=600 #need because ipc.h require functions from XOPEN_SOURCE library

all: vmappl mmanage.o


vmappl.o: vmappl.c vmappl.h
	$(CC) $(CCFLAGS) -c -o vmappl.o vmappl.c

vmaccess.o: vmaccess.c vmaccess.h
	$(CC) $(CCFLAGS) -c -o vmaccess.o vmaccess.c

vmappl: vmaccess.o vmappl.o
	$(CC) $(LDFLAGS) -o vmappl vmaccess.o vmappl.o

mmanage.o: mmanage.c mmanage.h
	$(CC) $(CCFLAGS) $(SOURCE_FLAG) -c -o mmanage.o mmanage.c

clean:
	rm vmappl *.o
