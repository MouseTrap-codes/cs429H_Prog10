#include "tdmm.h"
#include <sys/mman.h>



alloc_strat_e stratChosen = FIRST_FIT; // default

void
t_init (alloc_strat_e strat)
{
  size_t size = 16384; // 4 pages of memory
  void* addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0); 
  stratChosen = strat;

  
  // call mmap
  // allocate 37 bytes to user
  // header info -> 20 bytes
  // take 57 -> use 20 for llnode struct and give the user 37
  // alignment
}

void *
t_malloc (size_t size)
{
  switch (stratChosen) {
    case FIRST_FIT:
      // first fit method
      break;

    case BEST_FIT:
      // best fit method
      break;
    
    case WORST_FIT:
      // worst fit method
      break;
    
    default:
      printf("unknown allocation strategy/allocation type not implemented.");
      break;
  }
  return NULL;
}

void* firstFit(size_t size) {

}

void
t_free (void *ptr)
{
  // TODO: Implement this
}

void
t_gcollect (void)
{
  // TODO: Implement this
}