/* File for vmaccess.c
 * BSP Gruppe 1
 * Sahoo, Morozov
 *
 * This file gives the vmappl.c access to the virtual memory.
 */

#include "vmaccess.h"

// TODO: connect to shared memory

// TODO: how to access vm_mem? -> like this
struct vmem_struct *vmem = NULL;	// Shared Memory with mmanager.c


#ifndef DEBUG_MESSAGES
#define DEBUG(A) 
#endif

#ifdef DEBUG_MESSAGES
#define DEBUG(A) (A)
#endif

// Usage: DEBUG(fprintf(stderr, "blubb bla bluff\n"));

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
    int result;
    // page und offset berechnen.
    int page = address / VMEM_PAGESIZE;
    int offset = address % VMEM_PAGESIZE;
    
    // verwendete page vermerken
    // damit im Falle eines Pagefaults
    // mmanage diese Page laden kann
    vmem->adm.req_pageno = page;
    
    int flags = vmem->pt.entries[page].flags;
    // check whether the page is currently loaded
    int req_page_is_loaded = ((flags & PTF_PRESENT) == PTF_PRESENT);
    
    
    if (!req_page_is_loaded) {
	// DEBUG(fprintf(stderr, "Pagefult for reading!\n"));
	kill(vmem->adm.mmanage_pid, SIGUSR1);
	sem_wait(&vmem->adm.sema);
    }
    
    result = read_page(page, offset);
    return result;
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

void write_page(int page, int offset, int data) {
    countUsed(page);
    int index = calcIndexFromPageOffset(page, offset);
    // DEBUG(fprintf(stderr, "Write: Page: %d Offset: %d Data: %d\n", page, offset, data));
    vmem->data[index] = data;
}

void countUsed(int page) {
    // if USED bit 1 is already set, then set the second used bit.
    int used_1_is_set = vmem->pt.entries[page].flags & PTF_USED;
    if(used_1_is_set) {
	set_bits(&(vmem->pt.entries[page].flags), PTF_USED1);
    }
    set_bits(&(vmem->pt.entries[page].flags), PTF_USED);
}

void set_bits(int *to_be_set, int bits) {
    *(to_be_set) |= bits;
}

void vmem_write(int address, int data) {
    vm_init_if_not_ready();
    // page und offset berechnen.
    int page = address / VMEM_PAGESIZE;
    int offset = address % VMEM_PAGESIZE;
    
    // verwendete page vermerken
    // damit im Falle eines Pagefaults
    // mmanage diese Page laden kann
    vmem->adm.req_pageno = page;
    
    int flags = vmem->pt.entries[page].flags;
    // check whether the page is currently loaded
    int req_page_is_loaded = ((flags & PTF_PRESENT) == PTF_PRESENT);
    
    
    if (!req_page_is_loaded) {
	// DEBUG(fprintf(stderr, "Pagefult for writing!\n"));
	kill(vmem->adm.mmanage_pid, SIGUSR1);
	sem_wait(&vmem->adm.sema);
    }
    
    write_page(page, offset, data);
}

void vm_init_if_not_ready() {
    if(vmem == NULL) {
        vm_init();
    }
}

void dump() {
    kill(vmem->adm.mmanage_pid,SIGUSR2);
}



