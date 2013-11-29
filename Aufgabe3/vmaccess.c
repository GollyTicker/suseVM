/* File for vmaccess.c
 * BSP Gruppe 1
 * Sahoo, Morozov
 *
 * This file gives the vmappl.c access to the virtual memory.
 */

#include "vmaccess.h"

// TODO: connect to shared memory

// dummy functionality

int memory[550];

int vmem_read(int address){
    return memory[address];
}


void vmem_write(int address, int data){
    memory[address]=data;
}



