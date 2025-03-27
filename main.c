#include "tdmm.h"
#include <stdio.h>
#include <doublell.h>

// Declare the external global block list.
extern BlockList* blockList;
int
main ()
{
  // Initialize the allocator with the FIRST_FIT strategy.
  t_init(FIRST_FIT);
  printf("Allocator initialized.\n");

  // Test 1: Allocate 100 bytes.
  void *p = t_malloc(100);
  if (p) {
      printf("Allocated 100 bytes at %p\n", p);
  } else {
      printf("Allocation of 100 bytes failed!\n");
  }

  // Free the allocated memory.
  t_free(p);
  printf("Freed 100-byte block.\n");

  // Test 2: Allocate 50 bytes.
  void *q = t_malloc(50);
  if (q) {
      printf("Allocated 50 bytes at %p\n", q);
  } else {
      printf("Allocation of 50 bytes failed!\n");
  }

  // Free the second allocation.
  t_free(q);
  printf("Freed 50-byte block.\n");

  return 0;
}