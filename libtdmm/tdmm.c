#include "tdmm.h"
#include <sys/mman.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "doublell.h"

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

Block* extendHeap(size_t size) {
  // Choose a new region size: either a minimum (e.g., 16384 bytes) or just big enough for the request.
  size_t minRegionSize = 16384;
  size_t newRegionSize = (size + sizeof(Block) + ALIGNMENT) * 2;
  
  // Allocate a new region with mmap.
  void* newRegion = mmap(NULL, newRegionSize, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (newRegion == MAP_FAILED) {
      perror("mmap in extend_heap failed");
      return NULL;
  }
  
  // Align the pointer (if needed) and initialize a new free block.
  Block* newBlock = (Block*) align_ptr(newRegion, ALIGNMENT);
  size_t overhead = (char*)newBlock - (char*)newRegion;
  newBlock->size = newRegionSize - overhead - sizeof(Block);
  newBlock->free = true;
  newBlock->prev = blockList->tail;  // Link this block to the end of our list.
  newBlock->next = NULL;
  
  // Insert newBlock at the end of the block list.
  if (blockList->tail) {
      blockList->tail->next = newBlock;
  } else {
      // In case blockList was empty.
      blockList->head = newBlock;
  }
  blockList->tail = newBlock;
  blockList->count++;
  
  return newBlock;
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
      firstFitBlock = extendHeap(size);
      if (!firstFitBlock) { // getting more memory from the OS through mmap did not work
        return NULL;
      }
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
  Block* bestFitBlock = NULL;
  Block* current = blockList->head;
    int index = 0;
    while (current != NULL) {
        if (current->free && current->size >= size) {
          if (!bestFitBlock || current->size < bestFitBlock->size) {
            bestFitBlock = current;
            break;
          }
        }
        current = current->next;
    }
  
  // get more memory if needed
  if (!bestFitBlock) {
    bestFitBlock = extendHeap(size);
    if (!bestFitBlock) {
      return NULL;
    }
  }

  // if the block is large enough, split it.
  if (bestFitBlock->size >= size + sizeof(Block) + ALIGNMENT) {
      Block* newBlock = (Block*)((char*)bestFitBlock + sizeof(Block) + size);
      newBlock->size = bestFitBlock->size - size - sizeof(Block);
      newBlock->free = true;
      newBlock->next = bestFitBlock->next;
      newBlock->prev = bestFitBlock;
      if (newBlock->next != NULL) {
          newBlock->next->prev = newBlock;
      } else {
          // if firstFitBlock was the tail, update the tail pointer.
          blockList->tail = newBlock;
      }
      bestFitBlock->next = newBlock;
      bestFitBlock->size = size;
  }

  // mark the block as allocated.
  bestFitBlock->free = false;

  // Return pointer to the usable memory (after the block header).
  return (void*)((char*)bestFitBlock + sizeof(Block));
}


void* worstFit(size_t size) {
  Block* worstFitBlock = NULL;
  Block* current = blockList->head;
    int index = 0;
    while (current != NULL) {
        if (current->free && current->size >= size) {
          if (!worstFitBlock || current->size > worstFitBlock->size) {
            worstFitBlock = current;
            break;
          }
        }
        current = current->next;
    }
  
  // get more memory if needed
  if (!worstFitBlock) {
    worstFitBlock = extendHeap(size);
    if (!worstFitBlock) {
      return NULL;
    }
  }

  // if the block is large enough, split it.
  if (worstFitBlock->size >= size + sizeof(Block) + ALIGNMENT) {
      Block* newBlock = (Block*)((char*)worstFitBlock + sizeof(Block) + size);
      newBlock->size = worstFitBlock->size - size - sizeof(Block);
      newBlock->free = true;
      newBlock->next = worstFitBlock->next;
      newBlock->prev = worstFitBlock;
      if (newBlock->next != NULL) {
          newBlock->next->prev = newBlock;
      } else {
          // if firstFitBlock was the tail, update the tail pointer.
          blockList->tail = newBlock;
      }
      worstFitBlock->next = newBlock;
      worstFitBlock->size = size;
  }

  // mark the block as allocated.
  worstFitBlock->free = false;

  // Return pointer to the usable memory (after the block header).
  return (void*)((char*)worstFitBlock + sizeof(Block));
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
      ptr = bestFit(size);
      break;
    
    case WORST_FIT:
      ptr = worstFit(size);
      break;
    
    case BUDDY:
      ptr = worstFit(size);
      break;
    
    default:
      // printf("unknown allocation strategy/allocation type not implemented.");
      break;
  }
  return ptr;
}


void 
t_free (void *ptr) {
  if (!ptr) return; 

  // step 1: Get the block header from the user pointer.
  Block *block = (Block *)((char *)ptr - sizeof(Block));

  // step 2: Mark the block as free
  block->free = true;

  // step 3: Coalesce with previous block if it's free
  if (block->prev && block->prev->free) {
      Block *prev = block->prev;
      // merge current block into previous block:
      prev->size += sizeof(Block) + block->size;
      prev->next = block->next;
      if (block->next) {
          block->next->prev = prev;
      } else {
          // update tail pointer if needed.
          blockList->tail = prev;
      }
      block = prev;  // use the merged block for further coalescing.
  }

  // step 4: coalesce with next block if it's free.
  if (block->next && block->next->free) {
      Block *next = block->next;
      block->size += sizeof(Block) + next->size;
      block->next = next->next;
      if (next->next) {
          next->next->prev = block;
      } else {
          blockList->tail = block;
      }
  }
}

void
t_gcollect (void)
{
  // TODO: Implement this
}