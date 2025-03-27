#include "tdmm.h"
#include <stdio.h>
#include <doublell.h>

// declare the external global block list
extern BlockList* blockList;

void printBlockList(BlockList *list) {
  Block *current = list->head;
  printf("Block List (count = %zu):\n", list->count);
  while (current) {
      printf("  Block at %p: size=%zu, free=%s, prev=%p, next=%p\n",
             (void*)current,
             current->size,
             current->free ? "true" : "false",
             (void*)current->prev,
             (void*)current->next);
      current = current->next;
  }
  printf("\n");
}


int
main ()
{
  printf("Initializing allocator with FIRST_FIT...\n");
    t_init(FIRST_FIT);
    printBlockList(blockList);

    // test 1: multiple allocations
    printf("Test 1: Allocate three blocks (100, 150, 200 bytes)\n");
    void *a = t_malloc(100);
    void *b = t_malloc(150);
    void *c = t_malloc(200);
    printf("Allocated: a = %p, b = %p, c = %p\n", a, b, c);
    printBlockList(blockList);

    // test 2: free some blocks to force coalescing
    printf("Test 2: Free block b, then block a, then block c\n");
    t_free(b);
    printBlockList(blockList);
    t_free(a);
    printBlockList(blockList);
    t_free(c);
    printBlockList(blockList);

    // test 3: allocate after free to test splitting of a free region
    printf("Test 3: Allocate block d (80 bytes) and then block e (50 bytes)\n");
    void *d = t_malloc(80);
    printf("Allocated: d = %p\n", d);
    printBlockList(blockList);
    void *e = t_malloc(50);
    printf("Allocated: e = %p\n", e);
    printBlockList(blockList);

    // test 4: Allocate a large block to force extendHeap() call
    printf("Test 4: Allocate a large block (5000 bytes) to force heap extension\n");
    void *f = t_malloc(5000);
    printf("Allocated: f = %p\n", f);
    printBlockList(blockList);

    // test 5: Free blocks in a different order to see coalescing across extended regions
    printf("Test 5: Free block e then block f\n");
    t_free(e);
    printBlockList(blockList);
    t_free(f);
    printBlockList(blockList);

    // test 6: Allocate and free blocks in a loop
    printf("Test 6: Allocate 10 blocks of increasing sizes and then free them\n");
    void *array[10];
    for (int i = 0; i < 10; i++) {
        array[i] = t_malloc(100 + i * 10);
        printf("Allocated block[%d] = %p\n", i, array[i]);
    }
    printBlockList(blockList);
    // free even-indexed blocks first.
    for (int i = 0; i < 10; i += 2) {
        t_free(array[i]);
        printf("Freed block[%d] = %p\n", i, array[i]);
    }
    printBlockList(blockList);
    // then free odd-indexed blocks.
    for (int i = 1; i < 10; i += 2) {
        t_free(array[i]);
        printf("Freed block[%d] = %p\n", i, array[i]);
    }
    printBlockList(blockList);

    printf("All tests complete.\n");

  return 0;
}