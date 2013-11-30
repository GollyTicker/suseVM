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


// dummy functionality
int memory[550];

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
        perror("ftruncate failed!\n");
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

int vmem_read(int address){
    vm_init_if_not_ready();
    return memory[address];
}


void vmem_write(int address, int data){
    vm_init_if_not_ready();
    memory[address]=data;
}

void vm_init_if_not_ready() {
    if(vmem == NULL) {
        vm_init();
	
	// make a dump after init
	kill(vmem->adm.mmanage_pid, SIGUSR2);
    }
}



