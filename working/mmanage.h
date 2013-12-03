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

// for connection to shared memory
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>



/** Event struct for logging */
// struct for the logging event
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

void fetch_page(int pt_idx);

void store_page(int pt_idx);

void update_pt(int frame);

int find_remove_frame(void);

int use_algorithm(void);

int find_remove_fifo(void);

int find_remove_clock(void);

int find_remove_clock2(void);

void signal_proccessing_loop(void);

// checks if a SIGUSR1 was caught and calls page_fault()
void case_page_fault(void);

// opens pagefile and maybe fills
// it with random data for easier debugging
void init_pagefile(const char *pfname);

void open_logfile();

// destroy all data and structurs because
// the process is ending
void cleanup();

// returns whether all frames are already occupied
int frames_are_occupied();

// log everthing given in this logevent
void logger(struct logevent le);

// print debug statement that we noticed a
// signal and reset signal number
void noticed(char *msg);

void update_load(int frame);
void update_unload(int oldpage);

void page_fault();

void rotate_alloc_idx();

void dump_vmem_structure();

/* Misc */
#define MMANAGE_PFNAME "./pagefile.bin" /**< pagefile name */
#define MMANAGE_LOGFNAME "./logfile.txt"        /**< logfile name */

#define MY_RANDOM_MOD 123

#ifdef FIFO
#define ALGO_STR "FIFO"
#endif

#ifdef CLOCK
#define ALGO_STR "CLOCK"
#endif

#ifdef CLOCK2
#define ALGO_STR "CLOCK2"
#endif

#define SEED_PF 123456        /**< Get reproducable pseudo-random numbers for
                           init_pagefile */

#define VOID_IDX -1

/* Edit to modify algo, or remove line and provide
 * -DVMEM_ALGO ... compiler flag*/
/* #define VMEM_ALGO VMEM_ALGO_FIFO */

#endif /* MMANAGE_H */
