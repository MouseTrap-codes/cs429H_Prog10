#include "tdmm.h"
#include <stdio>

int
main ()
{
  void* ptr1 = tmalloc(100);  
  void* ptr2 = tmalloc(100);
  void* ptr3 = tmalloc(100);  
  void* ptr4 = tmalloc(100);
  printf("malloced");

  tfree(ptr1);
  tfree(ptr2);
  tfree(ptr3);
  tfree(ptr4);
  printf("freed");

  return 0;
}