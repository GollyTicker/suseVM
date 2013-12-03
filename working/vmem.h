/* File: vmem.h
 * Global Definitions for BSP3 sample solution
 * Model of virtual memory management
 */

#ifndef VMEM_H
#define VMEM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHMKEY          "/vmem.h"
#define SHMPROCID       'C'

typedef unsigned int Bmword;    /* Frame bitmap */

/* Sizes */
// number of ints each process is offered
#define VMEM_VIRTMEMSIZE 1024   /* Process address space / items */

// Shared Memory(RAM) size in number of ints
#define VMEM_PHYSMEMSIZE  128   /* Physical memory / items */

// each page consists of 8 ints
#define VMEM_PAGESIZE       8   /* Items per page */

// Number of Pages for our process adress space
#define VMEM_NPAGES     (VMEM_VIRTMEMSIZE / VMEM_PAGESIZE)      /* Total 
                                                                   number 
                                                                   of
                                                                   pages 
                                                                 */

// Number of Frames in Sahred Memory
#define VMEM_NFRAMES (VMEM_PHYSMEMSIZE / VMEM_PAGESIZE) /* Number of
                                                           available
                                                           frames */

                                                           
#define VMEM_LASTBMMASK (~0U << (VMEM_NFRAMES % (sizeof(Bmword) * 8)))
#define VMEM_BITS_PER_BMWORD     (sizeof(Bmword) * 8)
#define VMEM_BMSIZE     ((VMEM_NFRAMES - 1) / VMEM_BITS_PER_BMWORD + 1)


/* Page Table */
// flags each pagetable entry can have

// says whether the page is currently loaded or not.
#define PTF_PRESENT     1

#define PTF_CHANGED       2       /* store: need to write */

// CLOCK ALGO 1 und 2 - the first USED BIT
#define PTF_USEDBIT1        4       /* For clock algo only */

// CLOCK ALGO 2 - the second USED BIT
#define PTF_USEDBIT2       8       /* For clock2 algo only */

// Each pagetable entry saves the flags and the reference to it's
// frame if it's currently loaded.
struct pt_entry {
    int flags;                  /* see defines above */
    int frame;                  /* Frame idx */
};

// admin is used to communicate inbetween
// processes and to store globl counting data.
struct vmem_adm_struct {
    
    // number of the currently used frames in shared memory
    int size;
    
    // mmanage saves its process id here
    // for other processes to know to which the 
    // signal is supposed to be sent.
    pid_t mmanage_pid;
    
    int shm_id;
    
    // Semaphor for the waiting during Pagefault
    sem_t sema;                 /* Coordinate acces to shm */
    int req_pageno;             /* Number of requested page */
    int next_alloc_idx;         /* Next frame to allocate (FIFO, CLOCK) 
                                 */
    int pf_count;               /* Page fault counter */
    
    
    Bmword bitmap[VMEM_BMSIZE]; /* 0 = free */
};

struct pt_struct {
    // dies array is the page table. each entry is an pt_entry
    struct pt_entry entries[VMEM_NPAGES];
    
    // framepage saves the page to the current
    // frames tot easier access.
    int framepage[VMEM_NFRAMES];        /* pages on frame */
};

/* This is to be located in shared memory */
// The Shareed Memory contains the PAgetable, the admin
// and the currently loaded data.
struct vmem_struct {
    struct vmem_adm_struct adm;
    struct pt_struct pt;
    int data[VMEM_NFRAMES * VMEM_PAGESIZE];
};

// Size of the shared memory for initialization
#define SHMSIZE (sizeof(struct vmem_struct))

#endif /* VMEM_H */
