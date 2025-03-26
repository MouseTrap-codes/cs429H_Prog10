#include "tdmm.h"

alloc_strat_e stratChosen = FIRST_FIT; // default

void
t_init (alloc_strat_e strat)
{
  stratChosen = strat;
  // TODO: Implement this
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