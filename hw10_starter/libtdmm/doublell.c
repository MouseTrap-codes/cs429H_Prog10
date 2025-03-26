#include <stdio.h>
#include <stdlib.h>
#include "doublell.h"

Block* createBlock(size_t size) {
    Block* newBlock = (Block*)malloc(sizeof(Block));
    if (!newBlock) {
        printf("Error: malloc failed!\n");
        return NULL;
    }

    newBlock->size = size;
    newBlock->free = true;
    newBlock->prev = NULL;
    newBlock->next = NULL;
    return newBlock;
}

BlockList* createBlockList() {
    BlockList* list = (BlockList*)malloc(sizeof(BlockList));
    if (!list) {
        printf("Error: malloc failed!\n");
        return NULL;
    }

    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
    return list;
}

void insertBlockFront(BlockList* list, Block* block) {
    block->next = list->head;
    block->prev = NULL;

    if (list->head != NULL)
        list->head->prev = block;
    else
        list->tail = block;

    list->head = block;
    list->count++;
}

void insertBlockBack(BlockList* list, Block* block) {
    block->next = NULL;
    block->prev = list->tail;

    if (list->tail != NULL)
        list->tail->next = block;
    else
        list->head = block;

    list->tail = block;
    list->count++;
}

void insertBlockAfter(BlockList* list, Block* target, Block* newBlock) {
    if (!target) return;

    newBlock->prev = target;
    newBlock->next = target->next;

    if (target->next != NULL)
        target->next->prev = newBlock;
    else
        list->tail = newBlock;

    target->next = newBlock;
    list->count++;
}

void removeBlock(BlockList* list, Block* block) {
    if (!block) return;

    if (block->prev != NULL)
        block->prev->next = block->next;
    else
        list->head = block->next;

    if (block->next != NULL)
        block->next->prev = block->prev;
    else
        list->tail = block->prev;

    free(block);
    list->count--;
}

// this is only for garbage collection but obviously we cannot use this cause it uses "free" -> don't think we have to implement garbage collection though
// void destroyBlockList(BlockList* list) {
//     Block* current = list->head;
//     while (current != NULL) {
//         Block* next = current->next;
//         free(current);
//         current = next;
//     }
//     free(list);
// }

void printBlockList(BlockList* list) {
    Block* current = list->head;
    int index = 0;
    printf("Block List:\n");
    while (current != NULL) {
        printf("  [%d] size=%zu, free=%s\n", index++, current->size, current->free ? "true" : "false");
        current = current->next;
    }
}
