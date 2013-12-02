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
	vm_init_if_not_ready();
	
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
    countUsed(page);
    int index = calcIndexFromPageOffset(page, offset);
    // DEBUG(fprintf(stderr, "Reading: Page: %d Offset: %d\n", page, offset));
    return vmem->data[index];
}

int calcIndexFromPageOffset(int page, int offset) {
    return (vmem->pt.entries[page].frame*VMEM_PAGESIZE) + offset;
}

void countUsed(int page) {
    // if USED bit 1 is already set, then set the second used bit.
    int used_1_is_set = vmem->pt.entries[page].flags & PTF_USED;
    if(used_1_is_set) {
	vmem->pt.entries[page].flags |= PTF_USED1;
    }
    // the first used bit is always set
    vmem->pt.entries[page].flags |= PTF_USED;
}

void vmem_write(int address, int data) {
	vm_init_if_not_ready();
	
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
    countUsed(page);
    
    // mark the change and to make sure it'll be updated
    // into the pagefile.bin
    vmem->pt.entries[page].flags |= PTF_DIRTY;
    
    int index = calcIndexFromPageOffset(page, offset);
    // DEBUG(fprintf(stderr, "Write: Page: %d Offset: %d Data: %d\n", page, offset, data));
    vmem->data[index] = data;
}


void vm_init_if_not_ready() {
    if(vmem == NULL) {
        vm_init();
    }
}

void dump() {
    kill(vmem->adm.mmanage_pid,SIGUSR2);
}
