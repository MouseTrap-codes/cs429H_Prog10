#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Define the structure of Node
typedef struct Block {
    size_t size; // size of memory block
    bool free; // whether or not the block is free (true or false)
    struct Node* prev; // previous block
    struct Node* next; // next block
} Block;

Block* createBlock(size_t size) {
    Block* newBlock = (Block*)malloc(sizeof(Block));
    newBlock->size = size;

    // Initially assigning the next and prev pointers to NULL
    newBlock->next = NULL;
    newBlock->prev = NULL;

    return newBlock;
}