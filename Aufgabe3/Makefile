CC := /usr/bin/gcc
CCFLAGS := -pthread -g -lpthread -Wall -std=c99
LDFLAGS := -pthread

all: vmappl mmanage.o


vmappl.o: vmappl.c
	$(CC) $(CCFLAGS) -c -o vmappl.o vmappl.c

vmaccess.o: vmaccess.c
	$(CC) $(CCFLAGS) -c -o vmaccess.o vmaccess.c

vmappl: vmaccess.o vmappl.o
	$(CC) $(LDFLAGS) -o vmappl vmaccess.o vmappl.o

mmanage.o: mmanage.c
	$(CC) $(CCFLAGS) -c -o mmanage.o mmanage.c

clean:
	rm vmappl *.o
