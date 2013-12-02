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
// adressraum das den prozesen vorgegaukelt wird
#define VMEM_VIRTMEMSIZE 1024   /* Process address space / items */

// unser Shared Memory(RAM)
#define VMEM_PHYSMEMSIZE  128   /* Physical memory / items */

// jede page hat 8 speicherstellen
#define VMEM_PAGESIZE       8   /* Items per page */

// Anzahl der Pages in unserem Prozessadressraum
#define VMEM_NPAGES     (VMEM_VIRTMEMSIZE / VMEM_PAGESIZE)      /* Total 
                                                                   number 
                                                                   of
                                                                   pages 
                                                                 */

// Anzahl der PAges im Shared Memory/RAM
#define VMEM_NFRAMES (VMEM_PHYSMEMSIZE / VMEM_PAGESIZE) /* Number of
                                                           available
                                                           frames */

                                                           

// wir haben diexse hier nicht verwendet/gebraucht
#define VMEM_LASTBMMASK (~0U << (VMEM_NFRAMES % (sizeof(Bmword) * 8)))
#define VMEM_BITS_PER_BMWORD     (sizeof(Bmword) * 8)
#define VMEM_BMSIZE     ((VMEM_NFRAMES - 1) / VMEM_BITS_PER_BMWORD + 1)


/* Page Table */
// Pro Page wird in einem Int in den lowest vier
// bits folgendes gespeichert

// ist die page gerade im arbeitsspeicher?
#define PTF_PRESENT     1

// hat die page sich veraendert und muss gespeichert werden?
#define PTF_DIRTY       2       /* store: need to write */

// CLOCK ALGO 1 und 2 - das erste USED BIT
#define PTF_USED        4       /* For clock algo only */

// CLOCK ALGO 2 - das zweite USED BIT
#define PTF_USED1       8       /* For clock2 algo only */

// ein eintrag in der Pagetable
// gepeichert sind die flags(PRESENT, USEDBITS etc...)
// sowie der index des dazugehoerigen Frames
// falls die Page gerade  geladen ist.
struct pt_entry {
    int flags;                  /* see defines above */
    int frame;                  /* Frame idx */
};

// administrative Struktur im Shared Memory
// die wird zur Kommunikation zwischen
// vmaccess.c und mmanage.c verwendet.
// Vor dem sem_wait schreibt vmaccess.c
// die Informationen (z.B. welhc ePage soll geladen werden)
// in diese Struktur. mmanage.c wird dann
// nachher diese Information lesen und entsprechend verarbeiten
struct vmem_adm_struct {
    
    // Anzahl der besetzten Frames im RAM/Shared Memory
    // beginnt bei 0 und geht im Laufe des Programms 0..1..2 .. VMEM_NFRAMES
    int size;
    
    pid_t mmanage_pid;  // fuer den kill(pid, SIGUSR1) - Befehl
    
    int shm_id;
    
    // Semaphor fuer das Warten beim Pagefault
    sem_t sema;                 /* Coordinate acces to shm */
    int req_pageno;             /* Number of requested page */
    int next_alloc_idx;         /* Next frame to allocate (FIFO, CLOCK) 
                                 */
    int pf_count;               /* Page fault counter */
    
    // hm....
    Bmword bitmap[VMEM_BMSIZE]; /* 0 = free */
};

struct pt_struct {
    // dieses array ist das page table.
    // zu jedem page gibt es den entsprchenden Eintrag(entry)
    struct pt_entry entries[VMEM_NPAGES];
    
    // in dieser Tabelle steht, zu welchem
    // Frame welche Page zugeordnet ist.
    // Dies macht es insofern einfacher,
    // da wir nun nicht ueber alle Pages iterieren muessen
    // um zu einem Frame dessen Page zu finden.
    int framepage[VMEM_NFRAMES];        /* pages on frame */
};

/* This is to be located in shared memory */
// Im shared memory sind daher einfach
// das Pagetable, die administrative Struktur
// und die derzeit geladenen zu finden.
struct vmem_struct {
    struct vmem_adm_struct adm;
    struct pt_struct pt;
    int data[VMEM_NFRAMES * VMEM_PAGESIZE];
};

// Groesse der Shared Memory Struktur
// fuer die initialisierung.
#define SHMSIZE (sizeof(struct vmem_struct))

#endif /* VMEM_H */
