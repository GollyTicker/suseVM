 
#include <stdio.h>
#include <stdlib.h>
#include "vmaccess.h"
#include "vmem.h"


int main(void){
  
  dump();
  
  int address = 14;
  int data = 50;
  vmem_write(address, data);  
  
  int readtest = vmem_read(address);
  
  if(readtest == data){
    fprintf(stderr, "Readtest Success!\n");
  }else{
    fprintf(stderr, "Readtest Failed!\n");
  }
  fprintf(stderr, "wrote %d data to address %d with the read result %d", data, address, readtest);
  
  dump();
}