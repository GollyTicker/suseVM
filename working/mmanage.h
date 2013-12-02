/* Definitions for virtual memory management model
 * File: mmanage.h
 *
 * Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * 2010
 */
#ifndef MMANAGE_H
#define MMANAGE_H
#include "vmem.h"
#include <limits.h>

/** Event struct for logging */
// Die zu loggenden Daten werden jedesmal
// hier reingeschrieben bevordie
// loggerfunktion aufgerufen wird.
struct logevent {
    int req_pageno;
    int replaced_page;
    int alloc_frame;
    int pf_count;
    int g_count;
};

// before anything happens,
// the main method here is called
// which opens the pagefile and the logfile
// initializes the virtual memory (shared memory)
// and then wiats for the signals

/* Prototypes */
void sighandler(int signo);

void vmem_init(void);

void allocate_page(void);

void fetch_page(int pt_idx);

void store_page(int pt_idx);

void update_pt(int frame);

int find_remove_frame(void);

int find_remove_fifo(void);

int find_remove_lfu(void);

int find_remove_clock(void);

int search_bitmap(void);

int find_free_bit(Bmword bmword, Bmword mask);

void init_pagefile(const char *pfname);

void cleanup(void);

void logger(struct logevent le);

void dump_pt(void);

void init_semaphor(void);

/* Misc */
#define MMANAGE_PFNAME "./pagefile.bin" /* pagefile name */
#define MMANAGE_LOGFNAME "./logfile.txt"        /* logfile name */

#define VMEM_ALGO_FIFO  0
#define VMEM_ALGO_LFU   1
#define VMEM_ALGO_CLOCK 2

#define SEED 120521        /* Get reproducable pseudo-random numbers for
                           init_pagefile */

#define VOID_IDX -1

/* Uncomment next #define and edit to modify algo,
 * or remove line and provide
 * -DVMEM_ALGO ... compiler flag*/
/* #define VMEM_ALGO VMEM_ALGO_FIFO */

#endif /* MMANAGE_H */
