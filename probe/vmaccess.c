#include "vmaccess.h"
#include "vmem.h"
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>  

struct vmem_struct *vmem = NULL;								/* Shared Memory */


/* Verbinden mit den Shared Memory */
void vm_init(void) {
int fd = shm_open(SHMKEY, O_RDWR, S_IRUSR | S_IWUSR);			/* Connection between a shred memory object and a file discriptor */ 
if(!fd) {
	perror("shm_open failed\n");
	exit(EXIT_FAILURE);
}
#ifdef DEBUG_MESSAGES
else {
	fprintf(stderr, "shm_open succeeded\n");
}
#endif /* DEBUG_MESSAGES */

if( ftruncate(fd, sizeof(struct vmem_struct)) == -1) {			/* Setze Groesse */
	perror("ftruncate failed\n");
	exit(EXIT_FAILURE);
}
#ifdef DEBUG_MESSAGES
else {
	fprintf(stderr, "ftruncate succeeded\n");
}
#endif /* DEBUG_MESSAGES */

/* Struktur in den Speicher mappen */
vmem = mmap(NULL, sizeof(struct vmem_struct), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
if(!vmem) {
	perror("mmap failed\n");
	exit(EXIT_FAILURE);
}
#ifdef DEBUG_MESSAGES
else {
	fprintf(stderr, "mmap succeeded\n");
}
#endif /* DEBUG_MESSAGES */
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
