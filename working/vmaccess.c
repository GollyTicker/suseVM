// File for vmaccess.c
// This file gives vmappl.c the access to the virtual memory

#include "vmaccess.h"

// shared memory variable
struct vmem_struct *vmem = NULL;


#ifndef DEBUG_MESSAGES
#define DEBUG(A) 
#endif

#ifdef DEBUG_MESSAGES
#define DEBUG(A) (A)
#endif

// Usage: DEBUG(fprintf(stderr, "my message %d\n", count));

void vm_init(){
    // connect to shared memory
    int fd = shm_open(SHMKEY, O_RDWR, S_IRUSR | S_IWUSR); 
    if(!fd) {
        perror("shm_open failed!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "shm_open succeeded.\n"));
    }
    
    // Groesse des gesharten Memory setzten
    if( ftruncate(fd, sizeof(struct vmem_struct)) == -1) {
        perror("ftruncate failed! Make sure ./mmanage is running!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "ftruncate succeeded.\n"));
    }

    // mach den Shared Memory unter vmem verfuegbar
    vmem = mmap(NULL, sizeof(struct vmem_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(!vmem) {
        perror("mapping into vmem failed!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "mapping into vmem succeeded!\n"));
    }
}

int vmem_read(int address) {
	if(vmem == NULL) {							/* Pruefen ob eine Verbindung zum Shared Memory besteht*/
		vm_init();
	}
	int data;
	int page = address/VMEM_PAGESIZE;					/* Ermitlung der Pagenummer */
	int offset = address%VMEM_PAGESIZE;					/* Ermittlung des Offsets */

	vmem->adm.req_pageno = page;						/* Angeforderte Seite */
	
	sem_wait(&vmem->adm.sema);
	if((vmem->pt.entries[page].flags & PTF_PRESENT) == PTF_PRESENT) {	/* Ist die Seite im Speicher? */
		data = read_page(page, offset);					/* Wenn ja, Seite lesen */
	} else {								/* Wenn nein, Seite anfordern */
		kill(vmem->adm.mmanage_pid, SIGUSR1);				/* Sende Signal an mmanage*/
		sem_wait(&vmem->adm.sema);					/* Warte bis mmanage den Semaphor freigibt */
		data = read_page(page, offset);					/* Page lesen*/
	}
	sem_post(&vmem->adm.sema);
	return data;
}

int read_page(int page, int offset) {	
	vmem->pt.entries[page].count++;									/*Counter für LFU*/
	vmem->pt.entries[page].flags |= PTF_USED;							/* Used Flag setzen (CLOCK)*/
	return vmem->data[vmem->pt.entries[page].frame*VMEM_PAGESIZE + offset];/* Lesen */
}

void vmem_write(int address, int data) {
	if(vmem == NULL) {			//Überprüfen ob mit shared Memory verbunden
		vm_init();
	}
	int page = address/VMEM_PAGESIZE;
	int offset = address%VMEM_PAGESIZE;
	
	vmem->adm.req_pageno = page;
	
	sem_wait(&vmem->adm.sema);
	if((vmem->pt.entries[page].flags & PTF_PRESENT) == PTF_PRESENT) {
		write_page(page, offset, data);								/* Seite schreiben */
	} else {
		kill(vmem->adm.mmanage_pid, SIGUSR1);
		sem_wait(&vmem->adm.sema);
		write_page(page, offset, data);								/* Seite schreiben */
	}
	sem_post(&vmem->adm.sema);
}

void write_page(int page, int offset, int data) {
	vmem->data[vmem->pt.entries[page].frame*VMEM_PAGESIZE + offset] = data;
	//vmem->pt.entries[vmem->pt.framepage[page]].count++;
	vmem->pt.entries[page].flags |= PTF_USED;
	vmem->pt.entries[page].flags |= PTF_DIRTY;
}
