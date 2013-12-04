/* Description: Memory Manager BSP3*/
/* Prof. Dr. Wolfgang Fohl, HAW Hamburg */
 /* 
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

struct vmem_struct *vmem = NULL;
FILE *pagefile = NULL;
FILE *logfile = NULL;
int signal_number = 0;

int shared_memory_file_desc;


#ifndef DEBUG_MESSAGES
#define DEBUG(A) 
#endif

#ifdef DEBUG_MESSAGES
#define DEBUG(A) (A)
#endif

// Usage: DEBUG(fprintf(stderr, "hallo!! %d\n", die_nummer));


int
 main(void)
{
    
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
        perror("Error installing signal handler for USR1!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR1 handler successfully installed.\n"));
    }

    if(sigaction(SIGUSR2, &sigact, NULL) != 0) {
        perror("Error installing signal handler for USR2!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "USR2 handler successfully installed.\n"));
    }

    if(sigaction(SIGINT, &sigact, NULL) != 0) {
        perror("Error installing signal handler for INT!\n");
        exit(EXIT_FAILURE);
    }
    else {
        DEBUG(fprintf(stderr, "INT handler successfully installed.\n"));
    }
    
    signal_proccessing_loop();
    
    exit(EXIT_SUCCESS);
}

void signal_proccessing_loop() {
    fprintf(stderr, "Memory Manager at pid(%d).\n", getpid());
    char *algo_str = ALGO_STR;
    fprintf(stderr, "Memory Manager is using %s algorithm. Active...\n", algo_str);
    while(1) {
	signal_number = 0;
	pause();
	if(signal_number == SIGUSR2) {
	  char *msg = "Signal recieved(SIGUSR2): dumping virtual memory.\n";
	  noticed(msg);
	  dump_vmem_structure();
	}
	else if(signal_number == SIGINT) {
	  char *msg = "Signal recieved(SIGINT): Quitting...\n";
	  noticed(msg);
	  cleanup();
	  break;
	}
    }
}

void page_fault() {
    int page_unloaded = VOID_IDX;
    int new_frame = VOID_IDX;
    int req_page = vmem->adm.req_pageno;
    
    DEBUG(fprintf(stderr, "\n<=== Pagefault ===>\n"));
    // A Pagefault has occured
    DEBUG(fprintf(stderr, "Requested Page: %d\n", req_page));
    
    vmem->adm.pf_count++;
    
    new_frame = find_remove_frame();
    
    page_unloaded = vmem->pt.framepage[new_frame];
    
    if( frames_are_occupied() ) {
	store_page(page_unloaded);
    }
    update_pt(new_frame);
    
    fetch_page(req_page);
    
    
    // make Logs
    struct logevent le;
    le.req_pageno = vmem->adm.req_pageno;
    le.replaced_page = page_unloaded;
    le.alloc_frame = new_frame;
    le.pf_count = vmem->adm.pf_count;
    le.g_count = 0;
    logger(le);
    
    DEBUG(fprintf(stderr, "Page loaded. pf_count: %d\n", vmem->adm.pf_count));
    
    // free the calling process
    sem_post(&vmem->adm.sema);
}

void sighandler(int signo) {
    signal_number = signo;
    
    // page fault has to be processed inside sig_handler
    case_page_fault();
}

void case_page_fault() {
    if(signal_number == SIGUSR1) {
	char *msg = "Signal recieved(SIGUSR1): Processing Pagefault\n";
	noticed(msg);
	page_fault();
    }
}

void dump_vmem_structure() {
    fprintf(stderr, " <========== DUMP OF VMEM =========> \n");
    fprintf(stderr, "Administrative Structures:\n");
    fprintf(stderr, "Filled: %d, Next_request: %d pf_count: %d Next_alloc_idx: %d\n",
	    vmem->adm.size, vmem->adm.req_pageno, vmem->adm.pf_count, vmem->adm.next_alloc_idx);
    fprintf(stderr, " <========== Data in vmem =========> \n");
    fprintf(stderr, "(index, data)\n");
    for(int i = 0; i < (VMEM_NFRAMES * VMEM_PAGESIZE); i++) {
	fprintf(stderr, "(%d, %d) \n", i, vmem->data[i]);
    }
}

void cleanup(){
    // delete shared memory
    munmap(vmem, SHMSIZE);
    close(shared_memory_file_desc);
    shm_unlink(SHMKEY);
    
    // close files
    fclose(logfile);
    fclose(pagefile);
    
    printf("Quit!\n");
}

void noticed(char *msg) {
    DEBUG(fprintf(stderr, msg));
    signal_number = 0;
}

int find_remove_frame(){
    int frame = VOID_IDX;
    if(!frames_are_occupied()) {
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
	exit(EXIT_FAILURE);
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
    rotate_alloc_idx();
    return frame;
}

// rotates the pointer to the next alloc in fifo clock and clock2
void rotate_alloc_idx() {
    vmem->adm.next_alloc_idx++;
    vmem->adm.next_alloc_idx%=(VMEM_NFRAMES);
}

int find_remove_clock() {
    int frame = VOID_IDX;
    
    while( frame == VOID_IDX ) {
	int alloc_idx = vmem->adm.next_alloc_idx;
	int page_by_alloc_idx = vmem->pt.framepage[alloc_idx];
	int flags = vmem->pt.entries[page_by_alloc_idx].flags;
	int is_frame_flag_used = (flags & PTF_USEDBIT1) == PTF_USEDBIT1;
	
	if(is_frame_flag_used) {
	    vmem->pt.entries[page_by_alloc_idx].flags &= ~PTF_USEDBIT1;
	    rotate_alloc_idx();
	}
	else {
	    frame = alloc_idx;
	}
    }
    rotate_alloc_idx();
    
    return frame;
}

void store_page(int page) {
    // if the fragme wasnt changed then dont store it.
    if( ( vmem->pt.entries[page].flags & PTF_CHANGED ) == PTF_CHANGED ) {
	int frame = vmem->pt.entries[page].frame;
	// scrool to the position to write into
	fseek(pagefile, sizeof(int)*VMEM_PAGESIZE*page, SEEK_SET);
	int written_ints = fwrite(&vmem->data[VMEM_PAGESIZE*frame], sizeof(int), VMEM_PAGESIZE, pagefile);
	if(written_ints != VMEM_PAGESIZE) {
	    perror("Not everything could be written into the page!\n");
	    exit(EXIT_FAILURE);
	}
    }
}

void fetch_page(int page) {
    int frame = vmem->pt.entries[page].frame;
    // scrool to the position to read from
    fseek(pagefile, sizeof(int)*VMEM_PAGESIZE*page, SEEK_SET);
    int readen_ints = fread(&vmem->data[VMEM_PAGESIZE*frame], sizeof(int), VMEM_PAGESIZE, pagefile);
    if(readen_ints != VMEM_PAGESIZE) {
	perror("Not everything could be read!\n");
	exit(EXIT_FAILURE);
    }
}

int find_remove_clock2() {
    int frame = VOID_IDX;
    
    while( frame == VOID_IDX ) {
	int alloc_idx = vmem->adm.next_alloc_idx;
	int page_by_alloc_idx = vmem->pt.framepage[alloc_idx];
	int flags = vmem->pt.entries[page_by_alloc_idx].flags;
	int is_frame_flag_used = (flags & PTF_USEDBIT1) == PTF_USEDBIT1;
	
	// if the first USED bit is set, then either delete
	// the second used bit or the first used bit.
	// else use this frame
	if(is_frame_flag_used) {
	    int is_second_frame_flag_used = (flags & PTF_USEDBIT2) == PTF_USEDBIT2;
	    
	    // if the second USED bit is also set,
	    // then unset it. else simply unset the first USED bit
	    if( is_second_frame_flag_used ) {
		vmem->pt.entries[page_by_alloc_idx].flags &= ~PTF_USEDBIT2;
	    }
	    else {
		vmem->pt.entries[page_by_alloc_idx].flags &= ~PTF_USEDBIT1;
	    }
	    
	    // the counter is being rotated
	    // because the current observed frame
	    // wasn't taken
	    rotate_alloc_idx();
	}
	else {
	    frame = alloc_idx;
	}
    }
    rotate_alloc_idx();
    
    return frame;
}


void update_pt(int frame){
    // unset old page
    int oldpage = vmem->pt.framepage[frame];
    DEBUG(fprintf(stderr, "Update Table: Oldpage: %d OldFrame: %d\n", oldpage, frame));
    update_unload(oldpage);
    
    // update loaded state
    update_load(frame);
}

void update_unload(int oldpage) {
    // delete all flags
    vmem->pt.entries[oldpage].flags = 0;
    
    // delete the reference to the frame it was occuping
    vmem->pt.entries[oldpage].frame = VOID_IDX;
    
}

void update_load(int frame) {
    int req_page = vmem->adm.req_pageno;
    vmem->pt.framepage[frame] = req_page;
    vmem->pt.entries[req_page].frame = frame;
    vmem->pt.entries[req_page].flags |= PTF_PRESENT;
}

int frames_are_occupied() {
    return (vmem->adm.size >= VMEM_NFRAMES);
}

void init_pagefile(const char *pfname) {
    int NoOfElements = VMEM_NPAGES*VMEM_PAGESIZE;
    int data[NoOfElements];
    
    srand(SEED_PF);
    // fill with random data. using our own rand_mod
    for(int i=0; i < NoOfElements; i++) {
	data[i] = rand() % MY_RANDOM_MOD;
    }
    
    pagefile = fopen(pfname, "w+b");
    if(!pagefile) {
        perror("Error creating pagefile!\n");
        exit(EXIT_FAILURE);
    }
    
    int writing_result = fwrite(data, sizeof(int), NoOfElements, pagefile);
    if(!writing_result) {
        perror("Error creating pagefile!\n");
        exit(EXIT_FAILURE);
    }
    DEBUG(fprintf(stderr, "Pagefile created.\n"));
}

void open_logfile(){
    logfile = fopen(MMANAGE_LOGFNAME, "w");
    if(!logfile) {
        perror("Error creating logfile!\n");
        exit(EXIT_FAILURE);
    }
    DEBUG(fprintf(stderr, "Logfile created.\n"));
}


void vmem_init(){
    // http://linux.die.net/man/3/shm_open
    shared_memory_file_desc = shm_open(SHMKEY, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if( shared_memory_file_desc != 0 ) {
	perror("Shared Memory creation failed!\n");
	exit(EXIT_FAILURE);
    }
    if( ftruncate(shared_memory_file_desc, SHMSIZE) != 0 ) {
	perror("Shared Memory truncate creation failed!\n");
	exit(EXIT_FAILURE);
    }

    vmem = mmap(NULL, SHMSIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory_file_desc, 0);
    if( vmem != 0 ){
	perror("Shared Memory could not be mapped into 'vmem'!\n");
	exit(EXIT_FAILURE);
    }
    DEBUG(fprintf(stderr, "Shared Virtual Memory space created. Initializing....\n"));
    
    // fill vmem with intial NULL-Data
    vmem->adm.size = 0;
    vmem->adm.mmanage_pid = getpid();
    vmem->adm.shm_id = VOID_IDX;
    vmem->adm.req_pageno = VOID_IDX;
    vmem->adm.next_alloc_idx = 0;
    vmem->adm.pf_count = 0;
    
    // initialize Semaphore
    int sem = sem_init(&vmem->adm.sema, 1, 0);
    if( sem != 0 ) {
	perror("Semaphore initialization failed!\n");
	exit(EXIT_FAILURE);
    }
    
    // initialize Page Table
    for(int i = 0; i < VMEM_NPAGES; i++) {
	vmem->pt.entries[i].flags = 0;
	vmem->pt.entries[i].frame = VOID_IDX;
    }
    
    // initialise Framepage
    for(int i = 0; i < VMEM_NFRAMES; i++) {
	vmem->pt.framepage[i] = VOID_IDX;
    }
      
    // initialize data
    for(int i = 0; i < (VMEM_NFRAMES * VMEM_PAGESIZE); i++) {
	vmem->data[i] = VOID_IDX;
    }
    
    DEBUG(fprintf(stderr, "Virtual Memory sucessfully created and accessible.\n"));
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