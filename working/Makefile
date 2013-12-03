CC=/usr/bin/gcc
CCFLAGS := -Wall -std=c99 -pthread
LDFLAGS := -lpthread
LRT := -lrt
SOURCE_FLAGS := -D_XOPEN_SOURCE=600 # some libreries requier open source

CCFLAGS += -DFIFO
#CCFLAGS += -DCLOCK
#CCFLAGS += -DCLOCK2

#CCFLAGS += -DDEBUG_MESSAGES

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

clean:
	rm -f vmappl mmanage *.o logfile.txt pagefile.bin
