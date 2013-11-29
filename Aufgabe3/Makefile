CC := /usr/bin/gcc
CCFLAGS := -Wall -std=c99
LDFLAGS :=:wq

all: vmappl mmanage.o


vmappl.o: vmappl.c vmappl.h
	$(CC) $(CCFLAGS) -c -o vmappl.o vmappl.c

vmaccess.o: vmaccess.c vmaccess.h
	$(CC) $(CCFLAGS) -c -o vmaccess.o vmaccess.c

vmappl: vmaccess.o vmappl.o
	$(CC) $(LDFLAGS) -o vmappl vmaccess.o vmappl.o

mmanage.o: mmanage.c mmanage.h
	$(CC) $(CCFLAGS) -c -o mmanage.o mmanage.c

clean:
	rm vmappl *.o
