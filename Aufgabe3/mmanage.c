/* Description: Memory Manager BSP3*/
/* Prof. Dr. Wolfgang Fohl, HAW Hamburg */
/* Winter 2010/2011
 * 
 * This is the memory manager process that
 * works together with the vmaccess process to
 * mimic virtual memory management.
 *
 * The memory manager process will be invoked
 * via a SIGUSR1 signal. It maintains the page table
 * and provides the data pages in shared memory
 *
 * This process is initiating the shared memory, so
 * it has to be started prior to the vmaccess process
 *
 * TODO:
 * currently nothing
 * */

#include "mmanage.h"

struct vmem_struct *vmem = NULL;	// Shared Memory with vmaccess.c
FILE *pagefile = NULL;
FILE *logfile = NULL;
int signal_number = 0;
#ifndef DEBUG_MESSAGES
#define DEBUG(A) 
#endif

#ifdef DEBUG_MESSAGES
#define DEBUG(A) (A)
#endif

// Usage: DEBUG(fprintf(stderr, "blubb bla bluff\n"));


int main(void) {
    struct sigaction sigact;

    /* Init pagefile */
    init_pagefile(MMANAGE_PFNAME);

    /* Open logfile */
    open_logfile();

    /* Create shared memory and init vmem structure */
    vmem_init();

    /* Setup signal handler */
    /* Handler for USR1 */
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    if(sigaction(SIGUSR1, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR1");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR1 handler successfully installed\n"));
    }

    if(sigaction(SIGUSR2, &sigact, NULL) == -1) {
        perror("Error installing signal handler for USR2");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR2 handler successfully installed\n"));
    }

    if(sigaction(SIGINT, &sigact, NULL) == -1) {
        perror("Error installing signal handler for INT");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "INT handler successfully installed\n"));
    }

    /* Signal processing loop */
    signal_proccessing_loop();
    exit(EXIT_SUCCESS);
}

/* Your code goes here... */

void signal_proccessing_loop(){
    DEBUG(fprintf(stderr, "Memory Manager: pid(%d)\n", getpid()));
    DEBUG(fprintf(stderr, "Memory Manager running...\n"));
    while(1) {
	signal_number = 0;
	pause();
	if(signal_number == SIGUSR1) {  /* Page fault */
	  char *msg = "Processed SIGUSR1\n";
	  noticed(msg);
	  // handle pagefault
	  
	}
	else if(signal_number == SIGUSR2) {     /* PT dump */
	  char *msg = "Processed SIGUSR2\n";
	  noticed(msg);
	  // TODO: dump vmem structure
	  
	}
	else if(signal_number == SIGINT) {
	  char *msg = "Processed SIGINT\n";
	  noticed(msg);
	  // TODO: finalizese quiting
	  fclose(logfile);
	  // fclose(pagefile);
	  printf("Quit!\n");
	  break;
	}
	else {
	  DEBUG(fprintf(stderr, "Unknown Signal: %d\n", signal_number));
	  signal_number = 0;
	}
    }
}

void noticed(char *msg) {
	DEBUG(fprintf(stderr, msg));
	signal_number = 0;
}

void sighandler(int signo) {
    signal_number = signo;
}

void vmem_init(){
    // TODO: vmem_init();
    vmem = 1;
    if(!vmem) {
        perror("Error initialising vmem");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "vmem successfully created\n"));
    }
}

void init_pagefile(){
    
    // mit random fuellen
    int NoOfElements = VMEM_NPAGES*VMEM_PAGESIZE;
    for(int i=0; i < NoOfElements; i++) {
		data[i] = rand() % MY_RANDOM_MOD;										
	}
    
	pagefile = fopen(pfname, "w+b");
    if(!pagefile) {
        perror("Error creating pagefile");
        exit(EXIT_FAILURE);
    }
    
    int writing_result = fwrite(data, sizeof(int), NoOfElements, pagefile);
    if(!writing_result) {
        perror("Error creating pagefile");
        exit(EXIT_FAILURE);
    }
}

void open_logfile(){
    logfile = fopen(MMANAGE_LOGFNAME, "w");
    if(!logfile) {
        perror("Error creating logfile");
        exit(EXIT_FAILURE);
    }
}

/* Do not change!  */
void
logger(struct logevent le)
{
    fprintf(logfile, "Page fault %10d, Global count %10d:\n"
            "Removed: %10d, Allocated: %10d, Frame: %10d\n",
            le.pf_count, le.g_count,
            le.replaced_page, le.req_pageno, le.alloc_frame);
    fflush(logfile);
}

