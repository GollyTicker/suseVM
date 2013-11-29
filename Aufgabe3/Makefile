CC := /usr/bin/gcc
CCFLAGS := -pthread -g -lpthread -Wall -std=c99
LDFLAGS := -pthread

all: philosophen


main.o: main.c
	$(CC) $(CCFLAGS) -c -o main.o main.c

monitor.o: monitor.c
	$(CC) $(CCFLAGS) -c -o monitor.o monitor.c

philosophen: main.o monitor.o
	$(CC) $(LDFLAGS) -o philosophen main.o monitor.o

clean: 
	rm philosophen main.o monitor.o
