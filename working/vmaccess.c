

#include "vmaccess.h"

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
    
    // set size of shared memory
    if( ftruncate(fd, sizeof(struct vmem_struct)) == -1) {
        perror("ftruncate failed! Make sure ./mmanage is running!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "ftruncate succeeded.\n"));
    }

    // make shared memory with the variable vmem accissible
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
    
    // calculate page and offset
    int page = address / VMEM_PAGESIZE;
    int offset = address % VMEM_PAGESIZE;

    // mark request for the case of a page fault
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

void countUsed(int page) {
    // if USED bit 1 is already set, then set the second used bit.
    int used_1_is_set = (vmem->pt.entries[page].flags & PTF_USEDBIT1) == PTF_USEDBIT1;
    if(used_1_is_set) {
	vmem->pt.entries[page].flags |= PTF_USEDBIT2;
    }
    // the first used bit is always set
    vmem->pt.entries[page].flags |= PTF_USEDBIT1;
}

void vmem_write(int address, int data) {
    vm_init_if_not_ready();
    
    // calculate page and offset
    int page = address / VMEM_PAGESIZE;
    int offset = address % VMEM_PAGESIZE;
    
    // mark request for the case of a page fault
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

void write_page(int page, int offset, int data) {
    countUsed(page);
    
    // mark the change and to make sure it'll be updated
    // into the pagefile.bin
    vmem->pt.entries[page].flags |= PTF_CHANGED;
    
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
