/* Header file for vmappl.c
 * File: vmappl.h
 * Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * 2010
 */

#ifndef VMACCESS_H
#define VMACCESS_H

#include "vmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h> 

/** Connect to shared memory (key from vmem.h) */
void vm_init(void);

/** Read from "virtual" address */
int vmem_read(int address);

/** Write data to "virtual" address */
void vmem_write(int address, int data);

// init the virtual memory and connect to shared memory  if not done yet
void vm_init_if_not_ready();

// send kill command to mmanage for debugging
void dump();

void countUsed(int page);

void write_page(int page, int offset, int data);
int read_page(int page, int offset);
int calcIndexFromPageOffset(int page, int offset);

#endif
