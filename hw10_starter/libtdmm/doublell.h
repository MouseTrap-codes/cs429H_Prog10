#ifndef BLOCKLIST_H
#define BLOCKLIST_H

#include <stddef.h>
#include <stdbool.h>

// Define the structure of Block (Node)
typedef struct Block {
    size_t size;         // size of memory block
    bool free;           // whether or not the block is free
    struct Block* prev;  // previous block
    struct Block* next;  // next block
} Block;

// Doubly linked list wrapper
typedef struct BlockList {
    Block* head;
    Block* tail;
    size_t count;
} BlockList;

// Function declarations
BlockList* createBlockList();
Block* createBlock(size_t size);
void insertBlockFront(BlockList* list, Block* block);
void insertBlockBack(BlockList* list, Block* block);
void insertBlockAfter(BlockList* list, Block* target, Block* newBlock);
void removeBlock(BlockList* list, Block* block);
void destroyBlockList(BlockList* list);
void printBlockList(BlockList* list);

#endif