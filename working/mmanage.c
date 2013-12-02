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


int
 main(void)
{
    srand(SEED_PF);
    
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
    if(sigaction(SIGUSR1, &sigact, NULL) != 0) {
        perror("Error installing signal handler for USR1");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR1 handler successfully installed\n"));
    }

    if(sigaction(SIGUSR2, &sigact, NULL) != 0) {
        perror("Error installing signal handler for USR2");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR2 handler successfully installed\n"));
    }

    if(sigaction(SIGINT, &sigact, NULL) != 0) {
        perror("Error installing signal handler for INT");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "INT handler successfully installed\n"));
    }
    
    /* Signal processing loop */
    while(1) {
        signal_number = 0;
        pause();
        if(signal_number == SIGUSR1) {  /* Page fault */
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGUSR1\n");
#endif /* DEBUG_MESSAGES */
            signal_number = 0;
        }
        else if(signal_number == SIGUSR2) {     /* PT dump */
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGUSR2\n");
#endif /* DEBUG_MESSAGES */
            signal_number = 0;
        }
        else if(signal_number == SIGINT) {
#ifdef DEBUG_MESSAGES
            fprintf(stderr, "Processed SIGINT\n");
#endif /* DEBUG_MESSAGES */
        }
    }


    return 0;
}

void sighandler(int signo) {
    
	if(signo == SIGUSR1) {
				
		struct logevent le;
		int frame;
		int oldpage;
		
		/* Page Fault Counter erhoehen */
		vmem->adm.pf_count++;
		
#ifdef DEBUG_MESSAGES
		fprintf(stderr, "Requested Page: %d\n", vmem->adm.req_pageno);
#endif /* DEBUG_MESSAGES */
		
		/* freien frame suchen */
		frame = find_remove_frame();

#ifdef DEBUG_MESSAGES
		fprintf(stderr, "Free Frame: %d\n", frame);
#endif /* DEBUG_MESSAGES */
		oldpage = vmem->pt.framepage[frame];
			
		/* page speichern */
		if(vmem_is_full()) {
			if((vmem->pt.entries[oldpage].flags & PTF_DIRTY) == PTF_DIRTY) {
				store_page(vmem->pt.framepage[frame]);
			}	
		}
		
		/* Pagetable aktualisieren */
		update_pt(frame);
		
		/* Page aus Pagefile holen */
		fetch_page(vmem->adm.req_pageno);
		
		/* Logging */
		le.req_pageno = vmem->adm.req_pageno;
		le.replaced_page = oldpage;
		le.alloc_frame = frame;
		le.pf_count = vmem->adm.pf_count;
		
		le.g_count = 0;	// <- Was heißt g_count?
		
		logger(le);
		
        /* Semaphor freigeben */
		sem_post(&vmem->adm.sema);
	}
    
	if(signo == SIGUSR2) {
		int i;
		int j;
		int max_pf_idx = 300;
		
		printf("Data :\n");
		for(i=0; i<VMEM_NFRAMES * VMEM_PAGESIZE; i++) {
			printf("%d   %d\n", i, vmem->data[i]);
		}
      
		rewind(pagefile);
		printf("Page File: \n");
		for(i=0; i<max_pf_idx; i++) {
			fread(&j, sizeof(int), 1, pagefile);
			printf("%d   %d\n", i, j);
		}
    }
    
    if(signo == SIGINT) {
		/* clean up */
		munmap(vmem, SHMSIZE);
		close(shared_memory_file_desc);
		shm_unlink(SHMKEY);
		fclose(pagefile);
		fclose(logfile);
		exit(EXIT_SUCCESS);
    }
  }

int find_remove_frame(){
    int frame = VOID_IDX;
    if(!vmem_is_full()) {
	frame = vmem->adm.size;
	vmem->adm.size += 1;
	DEBUG(fprintf(stderr, "New Frame: %d (by free space)\n", frame));
    }
    else {
	frame = use_algorithm();
	DEBUG(fprintf(stderr, "New Frame: %d (by algorithm)\n", frame));
    }
    
    if(frame == VOID_IDX) {
	DEBUG(fprintf(stderr, "<================= FAIL returned Frame is -1 ==============>\n"));
    }
    return frame;
}

int use_algorithm() {
#ifdef FIFO
    return find_remove_fifo();
#endif
#ifdef CLOCK
    return find_remove_clock();
#endif
#ifdef CLOCK2
    return find_remove_clock2();
#endif
}

int find_remove_fifo() {
    int frame = vmem->adm.next_alloc_idx;
    // naechsten index weiter rotieren
    increment_alloc_idx(frame);
    return frame;
}

void increment_alloc_idx(int alloc_idx) {
    vmem->adm.next_alloc_idx++;
    vmem->adm.next_alloc_idx%=(VMEM_NFRAMES);
}


int find_remove_clock() {
    int frame;
    int done = 0;
    
    while(!done) {
	int alloc_idx = vmem->adm.next_alloc_idx;
	int frame_by_alloc_idx = vmem->pt.framepage[alloc_idx];
	int flags = vmem->pt.entries[frame_by_alloc_idx].flags;
	int is_frame_flag_used = (flags & PTF_USED) == PTF_USED;
	
	if(is_frame_flag_used) {	// if frame is used. unset it and continue to next.
	    vmem->pt.entries[frame_by_alloc_idx].flags &= ~PTF_USED;
	    increment_alloc_idx(alloc_idx);
	}
	else {
	    frame = alloc_idx;
	    done = 1;
	}
    }
    increment_alloc_idx(vmem->adm.next_alloc_idx);
    
    return frame;
}

void fetch_page(int pt_idx) {
	int count;
	int data_idx = vmem->pt.entries[pt_idx].frame;

	fseek(pagefile, sizeof(int)*VMEM_PAGESIZE*pt_idx, SEEK_SET);
	count = fread(&vmem->data[VMEM_PAGESIZE*data_idx], sizeof(int), VMEM_PAGESIZE, pagefile);
	if(count != VMEM_PAGESIZE) {
		perror("Konnte Page nicht lesen");
		exit(EXIT_FAILURE);
	}
}

void store_page(int pt_idx) {
	int count;
	int data_idx = vmem->pt.entries[pt_idx].frame;

	fseek(pagefile, sizeof(int)*VMEM_PAGESIZE*pt_idx, SEEK_SET);
	count = fwrite(&vmem->data[VMEM_PAGESIZE*data_idx], sizeof(int), VMEM_PAGESIZE, pagefile);
	if(count != VMEM_PAGESIZE) {
		perror("Konnte Page nicht schreiben");
		exit(EXIT_FAILURE);
	}
}


int find_remove_clock2() {
    int frame;
    int done = 0;
    
    while(!done) {
	int alloc_idx = vmem->adm.next_alloc_idx;
	int frame_by_alloc_idx = vmem->pt.framepage[alloc_idx];
	int flags = vmem->pt.entries[frame_by_alloc_idx].flags;
	int is_frame_flag_used = (flags & PTF_USED) == PTF_USED;
	
	// falls das used bit gesetzt ist. dann veringerre es (oder das zweite used bit)
	// sonst nehme diesen frame
	if(is_frame_flag_used) {
	    int is_second_frame_flag_used = (flags & PTF_USED1) == PTF_USED1;
	    // falls auch das zweite gesetzt ist,
	    // dann loesche das zweite bit. sonst loesche das erste bit
	    if( is_second_frame_flag_used ) {
		vmem->pt.entries[frame_by_alloc_idx].flags &= ~PTF_USED1;
	    }
	    else {
		vmem->pt.entries[frame_by_alloc_idx].flags &= ~PTF_USED;
	    }
	    
	    // counter erhoehen, um
	    // in der naechsten iteration den naechsten frame zu  betrachten
	    increment_alloc_idx(alloc_idx);
	}
	else {
	    frame = alloc_idx;
	    done = 1;
	}
    }
    increment_alloc_idx(vmem->adm.next_alloc_idx);
    
    return frame;
}

void update_pt(int frame) {
	int oldpage = vmem->pt.framepage[frame];

	vmem->pt.entries[oldpage].flags &= ~PTF_PRESENT;
	vmem->pt.entries[oldpage].flags &= ~PTF_DIRTY;
	vmem->pt.entries[oldpage].flags &= ~PTF_USED;
	vmem->pt.entries[oldpage].frame  =  VOID_IDX;

	vmem->pt.framepage[frame] = vmem->adm.req_pageno;
	vmem->pt.entries[vmem->adm.req_pageno].frame = frame;
	vmem->pt.entries[vmem->adm.req_pageno].flags |= PTF_PRESENT;
}

int vmem_is_full() {
    return (vmem->adm.size >= VMEM_NFRAMES);
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
    int sem = sem_init(&vmem->adm.sema, 1, 1);
    if(sem != 0) {
	perror("Semaphor initialization failed!");
	exit(EXIT_FAILURE);
    }
    
    // Page Table initialisieren
    for(int i = 0; i < VMEM_NPAGES; i++) {
	vmem->pt.entries[i].flags = 0;
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