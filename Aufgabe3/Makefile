CC := /usr/bin/gcc
CCFLAGS := -Wall -std=c99 -DDEBUG_MESSAGES -pthread
LRT := -lrt # lrt -> for shared memory open
LDFLAGS := -DDEBUG_MESSAGES -lpthread
SOURCE_FLAGS := -D_XOPEN_SOURCE=600 #need because ipc.h require functions from XOPEN_SOURCE library

all: vmappl mmanage

vmappl.o: vmappl.c vmappl.h
	$(CC) $(CCFLAGS) $(SOURCE_FLAGS) -c -o vmappl.o vmappl.c $(LRT)

vmaccess.o: vmaccess.c vmaccess.h
	$(CC) $(CCFLAGS) $(SOURCE_FLAGS) -c -o vmaccess.o vmaccess.c $(LRT)

vmappl: vmaccess.o vmappl.o
	$(CC) $(LDFLAGS) -o vmappl vmaccess.o vmappl.o $(LRT)

mmanage.o: mmanage.c mmanage.h
	$(CC) $(CCFLAGS) $(SOURCE_FLAGS) -c -o mmanage.o mmanage.c $(LRT)

mmanage: mmanage.o
	$(CC) $(LDFLAGS) -o mmanage mmanage.o $(LRT)
	
test.o: test.c test.h
	$(CC) $(CCFLAGS) $(SOURCE_FLAGS) -c -o test.o test.c $(LRT)

test: test.o
	$(CC) $(LDFLAGS) -o test test.o $(LRT)

clean:
	rm -f vmappl mmanage *.o logfile.txt pagefile.bin
