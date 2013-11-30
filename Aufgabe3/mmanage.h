/** Definitions for virtual memory management model
 * File: mmanage.h
 *
 * Prof. Dr. Wolfgang Fohl, HAW Hamburg
 * 2013
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

/** Prototypes */

// nimmt die signale SIGUSR1 SIGUSR2 und SIGINT
// and und schriebt sie in signal_number
void sighandler(int signo);

void vmem_init(void);

void allocate_page(void);

void fetch_page(int pt_idx);

void store_page(int pt_idx);

void update_pt(int frame);

int find_remove_frame(void);

int find_remove_fifo(void);

int find_remove_clock(void);

int find_remove_clock2(void);

int search_bitmap(void);


// ???!?
int find_free_bit(Bmword bmword, Bmword mask);

// opens pagefile and maybe fills
// it with random data for easier debugging
void init_pagefile();

// destroy all data and structurs because
// the process is ending
void cleanup(void);

// log everthing given in this logevent
void logger(struct logevent le);

// show 
void dump_pt(void);

void open_logfile();

void signal_proccessing_loop();

void noticed(char *msg);

/** Misc */
#define MMANAGE_PFNAME "./pagefile.bin" /**< pagefile name */
#define MMANAGE_LOGFNAME "./logfile.txt"        /**< logfile name */

#define VMEM_ALGO_FIFO  0
#define VMEM_ALGO_LRU   1
#define VMEM_ALGO_CLOCK 2
#define VMEM_ALGO_CLOCK2 3

#define SEED_PF 290913        /**< Get reproducable pseudo-random numbers for
                           init_pagefile */

#define VOID_IDX -1

/* Edit to modify algo, or remove line and provide
 * -DVMEM_ALGO ... compiler flag*/
/* #define VMEM_ALGO VMEM_ALGO_FIFO */

#endif /* MMANAGE_H */
