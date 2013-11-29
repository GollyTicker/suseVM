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
    // DEBUG(fprintf(stderr, "vm_init();"));
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
    }
}



