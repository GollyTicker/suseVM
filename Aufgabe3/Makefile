CC := /usr/bin/gcc
CCFLAGS := -pthread -g -lpthread -Wall -std=c99
LDFLAGS := -pthread

all: bin/vmappl #vmem


bin/vmappl.o: vmappl.c
	$(CC) $(CCFLAGS) -c -o bin/vmappl.o vmappl.c

bin/vmaccess.o: vmaccess.c
	$(CC) $(CCFLAGS) -c -o bin/vmaccess.o vmaccess.c

bin/vmappl: bin/vmaccess.o bin/vmappl.o
	$(CC) $(LDFLAGS) -o bin/vmappl bin/vmaccess.o bin/vmappl.o
	
clean:
	rm bin/*
