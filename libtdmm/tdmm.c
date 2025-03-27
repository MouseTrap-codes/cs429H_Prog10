#include "tdmm.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "doublell.h"

#define ALIGNMENT 16



#define ALIGNMENT 16

alloc_strat_e stratChosen = FIRST_FIT; // default
void* mmapRegion = NULL;

BlockList* blockList = NULL;

void* align_ptr(void* ptr, size_t alignment) {
  uintptr_t addr = (uintptr_t)ptr;
  uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
  return (void*)aligned;
}

void t_init(alloc_strat_e strat) {
  stratChosen = strat;

  size_t totalSize = 16384; // 4 pages of memory
  mmapRegion = mmap(NULL, totalSize, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (mmapRegion == MAP_FAILED) {
    perror("mmap failed");
    return;
  }

  // Reserve space for BlockList at the beginning of mmapRegion.
  // This avoids calling malloc.
  blockList = (BlockList*)mmapRegion;
  blockList->head = NULL;
  blockList->tail = NULL;
  blockList->count = 0;

  // Calculate where the user space starts, after the BlockList.
  void* metadataEnd = (char*)mmapRegion + sizeof(BlockList);
  void* userRegion = align_ptr(metadataEnd, ALIGNMENT);

  // create the first free block in the remaining memory
  Block* block = (Block*)userRegion;
  size_t overhead = (char*)userRegion - (char*)mmapRegion;
  block->size = totalSize - overhead;
  block->free = true;
  block->prev = NULL;
  block->next = NULL;

  // insert the block into blockList (which is stored in the mmap region)
  blockList->head = block;
  blockList->tail = block;
  blockList->count = 1;
}

void* firstFit(size_t size) {
  Block* firstFitBlock = NULL;
  Block* current = blockList->head;
  
  // find the first free block that fits the requested size.
  while (current != NULL) {
      if (current->free && current->size >= size) {
          firstFitBlock = current;
          break;
      }
      current = current->next;
  }
  
  // check if a suitable block was found.
  if (!firstFitBlock) {
      // Optionally, extend your mmap region or return NULL.
      return NULL;
  }
  
  // if the block is large enough, split it.
  if (firstFitBlock->size >= size + sizeof(Block) + ALIGNMENT) {
      Block* newBlock = (Block*)((char*)firstFitBlock + sizeof(Block) + size);
      newBlock->size = firstFitBlock->size - size - sizeof(Block);
      newBlock->free = true;
      newBlock->next = firstFitBlock->next;
      newBlock->prev = firstFitBlock;
      if (newBlock->next != NULL) {
          newBlock->next->prev = newBlock;
      } else {
          // if firstFitBlock was the tail, update the tail pointer.
          blockList->tail = newBlock;
      }
      firstFitBlock->next = newBlock;
      firstFitBlock->size = size;
  }
  
  // mark the block as allocated.
  firstFitBlock->free = false;
  
  // Return pointer to the usable memory (after the block header).
  return (void*)((char*)firstFitBlock + sizeof(Block));
}

void* bestFit(size_t size) {
  Block* bestFitBlock;
  bestFitBlock->size = 10000000000000;
  Block* current = blockList->head;
    int index = 0;
    while (current != NULL) {
        if (current->free && current->size >= size) {
          if (current->size < bestFitBlock->size) {
            bestFitBlock = current;
          }
        }
        break;
        current = current->next;
    }
  
  return bestFitBlock;
}


void* worstFit(size_t size) {
  Block* worstFitBlock;
  worstFitBlock->size = 0;
  Block* current = blockList->head;
    int index = 0;
    while (current != NULL) {
        if (current->size && current->size >= size) {
          if (current->size > worstFitBlock->size) {
            worstFitBlock = current;
          }
        }
        break;
        current = current->next;
    }
    
  return worstFitBlock; 
}



void *
t_malloc (size_t size)
{
  void* ptr = NULL;
  switch (stratChosen) {
    case FIRST_FIT:
      ptr = firstFit(size);
      break;

    case BEST_FIT:
      ptr = firstFit(size);
      break;
    
    case WORST_FIT:
      ptr = firstFit(size);
      break;
    
    case BUDDY:
      ptr = firstFit(size);
      break;
    
    default:
      // printf("unknown allocation strategy/allocation type not implemented.");
      break;
  }
  return ptr;
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