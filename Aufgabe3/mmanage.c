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

// http://linux.die.net/man/3/shm_open
int shared_memory_file_desc;

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
	  dump_vmem_structure();
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
    shared_memory_file_desc = shm_open(SHMKEY, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(!shared_memory_file_desc) {
	perror("Shared Memory creation failed!");
	exit(EXIT_FAILURE);
    }
    if(ftruncate(shared_memory_file_desc, SHMSIZE) != 0) {
	perror("Shared Memory creation(truncate) failed!");
	exit(EXIT_FAILURE);
    }

    vmem = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_file_desc, 0);
    if(!vmem){
	perror("Shared Memory konnte nicht in 'vmem' gemappt werden!");
	exit(EXIT_FAILURE);
    }
    DEBUG(fprintf(stderr, "vmem sucessfully created. Initializing....\n"));
    
    // fill vmem with intial NULL-Data
    vmem->adm.size = 0;										
    vmem->adm.mmanage_pid = getpid();
    vmem->adm.shm_id = VOID_IDX;
    vmem->adm.req_pageno = VOID_IDX;
    vmem->adm.next_alloc_idx = 0;
    vmem->adm.pf_count = 0;
    
    // Semaphor initialisieren
    int sem = sem_init(&vmem->adm.sema, 0, 0);
    if(sem != 0) {
	perror("Semaphor initialization failed!");
	exit(EXIT_FAILURE);
    }
    
    // Page Tabke initialisieren
    for(int i = 0; i < VMEM_NPAGES; i++) {
	vmem->pt.entries[i].flags = 0;
	
	// TODO: braucht man dies hier wirklich?
	// vmem->pt.entries[i].flags &= ~PTF_PRESENT;
	// vmem->pt.entries[i].flags &= ~PTF_DIRTY;
	// vmem->pt.entries[i].flags &= ~PTF_USED;

	vmem->pt.entries[i].frame = VOID_IDX;
    }
    
    // Fragepage initialisieren
    for(int i = 0; i < VMEM_NFRAMES; i++) {
	vmem->pt.framepage[i] = VOID_IDX;
    }
      
    // data initialisieren
    for(int i = 0; i < (VMEM_NFRAMES * VMEM_PAGESIZE); i++) {
	vmem->data[i] = VOID_IDX;
    }
    
    DEBUG(fprintf(stderr, "vmem sucessfully created and accessible!\n"));
}

void dump_vmem_structure() {
    // alle gespeicherten Daten ausgeben
    DEBUG(fprintf(stderr, " <========== Data in vmem =========> \n"));
    DEBUG(fprintf(stderr, "(index, data)\n"));
    for(int i = 0; i < (VMEM_NFRAMES * VMEM_PAGESIZE); i++) {
	fprintf(stderr, "(%d, %d) \n", i, vmem->data[i]);
    }
    
    DEBUG(fprintf(stderr, "Administrative Structures:\n"));
    DEBUG(fprintf(stderr, "filled: %d, next_request: %d pf_count: %d",
	    vmem->adm.size, vmem->adm.req_pageno, vmem->adm.pf_count));
}

void init_pagefile(const char *pfname) {
    int NoOfElements = VMEM_NPAGES*VMEM_PAGESIZE;
    int data[NoOfElements];
    // mit random fuellen. wir verwenden unser eigenes random mod
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
    DEBUG(fprintf(stderr, "Pagefile created!\n"));
}

void open_logfile(){
    logfile = fopen(MMANAGE_LOGFNAME, "w");
    if(!logfile) {
        perror("Error creating logfile");
        exit(EXIT_FAILURE);
    }
    DEBUG(fprintf(stderr, "Logfile created!\n"));
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

