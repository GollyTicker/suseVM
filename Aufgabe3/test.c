
#include "test.h"

int main(void){
    
    int address = 14;
    int data = 50;
    vmem_write(address, data);
    dump();  
    
    int readtest = vmem_read(address);
    
    if(readtest == data){
	fprintf(stderr, "Readtest Success!\n");
    }else{
	fprintf(stderr, "Readtest Failed!\n");
    }
   
    fprintf(stderr, "wrote %d data to address %d with the read result %d", data, address, readtest);
    
    dump();
}
