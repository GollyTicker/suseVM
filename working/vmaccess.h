/* Header file for vmappl.c
 * File: vmappl.h
 * Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * 2010
 */

#ifndef VMACCESS_H
#define VMACCESS_H

/* Connect to shared memory (key from vmem.h) */
void vm_init(void);

/* Read from "virtual" address */
int vmem_read(int address);

/* Write data to "virtual" address */
void vmem_write(int address, int data);

/* Hilfsfunktionen */
void write_page(int frame, int offset, int data);

int read_page(int frame, int offset);

#endif
