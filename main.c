#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "tdmm.h"      // Contains declarations for t_malloc, t_free, t_init, etc.
#include "doublell.h"  // Contains the Block and BlockList definitions

// Extern declarations for global variables defined in your allocator source.
extern BlockList* blockList;
extern void* mmapRegion;

#define CSV_FILENAME "allocator_report.csv"

// Global variable to store the current strategy as a string.
char current_strategy[16];

// Helper: Convert a string argument into an allocation strategy,
// and record the strategy name.
alloc_strat_e get_strategy(const char *str) {
    if (strcmp(str, "best") == 0) {
        strcpy(current_strategy, "BEST_FIT");
        return BEST_FIT;
    } else if (strcmp(str, "worst") == 0) {
        strcpy(current_strategy, "WORST_FIT");
        return WORST_FIT;
    } else if (strcmp(str, "sequential") == 0) {
        strcpy(current_strategy, "SEQUENTIAL");
        return SEQUENTIAL;
    } else if (strcmp(str, "random") == 0) {
        strcpy(current_strategy, "RANDOM");
        return RANDOM;
    }
    strcpy(current_strategy, "FIRST_FIT");
    return FIRST_FIT;
}

// Helper: Compute current memory utilization by iterating over the block list.
void get_memory_metrics(size_t *totalMemory, size_t *allocatedMemory, int *blockCount) {
    *totalMemory = 0;
    *allocatedMemory = 0;
    *blockCount = 0;
    Block *current = blockList ? blockList->head : NULL;
    while (current != NULL) {
        *totalMemory += current->size;
        if (!current->free)
            *allocatedMemory += current->size;
        (*blockCount)++;
        current = current->next;
    }
}

// Helper: Log an event to the CSV file.
// This now includes a new column "OverheadBytes".
void log_event(FILE *csv, const char *event, const char *operation, size_t size, void *ptr, double opTime) {
    size_t totalMemory = 0, allocatedMemory = 0;
    int blockCount = 0;
    get_memory_metrics(&totalMemory, &allocatedMemory, &blockCount);
    double utilization = (totalMemory > 0) ? ((double)allocatedMemory / totalMemory) * 100.0 : 0.0;
    
    // Calculate overhead as the size of BlockList plus each Block's overhead.
    size_t overhead = sizeof(BlockList) + blockCount * sizeof(Block);
    
    // CSV columns: Strategy, Event, Operation, BlockSize, Pointer, OpTime(s), TotalMemory, AllocatedMemory, Utilization(%), BlockCount, OverheadBytes
    fprintf(csv, "%s,%s,%s,%zu,%p,%.8f,%zu,%zu,%.2f,%d,%zu\n",
            current_strategy, event, operation, size, ptr, opTime,
            totalMemory, allocatedMemory, utilization, blockCount, overhead);
    fflush(csv);
}

int main(int argc, char *argv[]) {
    // Seed the random number generator for tests that use randomness.
    srand(time(NULL));

    // Open CSV file for logging results.
    FILE *csv = fopen(CSV_FILENAME, "w");
    if (!csv) {
        perror("Failed to open CSV file for writing");
        return EXIT_FAILURE;
    }
    // Write CSV header (include the OverheadBytes column).
    fprintf(csv, "Strategy,Event,Operation,BlockSize,Pointer,OpTime(s),TotalMemory,AllocatedMemory,Utilization(%%),BlockCount,OverheadBytes\n");

    // Determine allocation strategy from command-line argument.
    alloc_strat_e strategy = FIRST_FIT;
    if (argc > 1) {
        strategy = get_strategy(argv[1]);
    } else {
        strcpy(current_strategy, "FIRST_FIT");
    }

    // Display chosen strategy.
    printf("Initializing memory allocator with strategy: %s\n", current_strategy);

    // Record initialization event.
    clock_t initStart = clock();
    t_init(strategy);
    clock_t initEnd = clock();
    double initTime = (double)(initEnd - initStart) / CLOCKS_PER_SEC;
    log_event(csv, "Initialization", "t_init", 0, mmapRegion, initTime);

    clock_t start, end;
    double opTime;

    // ---------------------------
    // Sequential Allocation Tests
    // ---------------------------

    // Test 1: Allocate 100 bytes.
    start = clock();
    void *p1 = t_malloc(100);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    if (!p1) {
        fprintf(stderr, "Allocation of 100 bytes failed.\n");
        fclose(csv);
        return EXIT_FAILURE;
    }
    memset(p1, 'A', 100);
    log_event(csv, "Allocation", "t_malloc", 100, p1, opTime);
    printf("Allocated 100 bytes at %p\n", p1);

    // Test 2: Allocate 200 bytes.
    start = clock();
    void *p2 = t_malloc(200);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    if (!p2) {
        fprintf(stderr, "Allocation of 200 bytes failed.\n");
        fclose(csv);
        return EXIT_FAILURE;
    }
    memset(p2, 'B', 200);
    log_event(csv, "Allocation", "t_malloc", 200, p2, opTime);
    printf("Allocated 200 bytes at %p\n", p2);

    // Test 3: Free the first block.
    start = clock();
    t_free(p1);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    log_event(csv, "Deallocation", "t_free", 100, p1, opTime);
    printf("Freed block at %p\n", p1);

    // Test 4: Allocate 50 bytes (to potentially reuse the freed space).
    start = clock();
    void *p3 = t_malloc(50);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    if (!p3) {
        fprintf(stderr, "Allocation of 50 bytes failed.\n");
        fclose(csv);
        return EXIT_FAILURE;
    }
    memset(p3, 'C', 50);
    log_event(csv, "Allocation", "t_malloc", 50, p3, opTime);
    printf("Allocated 50 bytes at %p (possibly reusing freed space)\n", p3);

    // Free remaining blocks.
    start = clock();
    t_free(p2);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    log_event(csv, "Deallocation", "t_free", 200, p2, opTime);
    printf("Freed block at %p\n", p2);

    start = clock();
    t_free(p3);
    end = clock();
    opTime = (double)(end - start) / CLOCKS_PER_SEC;
    log_event(csv, "Deallocation", "t_free", 50, p3, opTime);
    printf("Freed block at %p\n", p3);

    // Additional Test: Allocate and free multiple blocks sequentially.
    #define NUM_BLOCKS 10
    void *blocks[NUM_BLOCKS];
    printf("Sequential Test: Allocating %d blocks of increasing size...\n", NUM_BLOCKS);
    for (int i = 0; i < NUM_BLOCKS; i++) {
        size_t size = (i + 1) * 10;  // sizes: 10, 20, ... 100 bytes.
        start = clock();
        blocks[i] = t_malloc(size);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        if (!blocks[i]) {
            fprintf(stderr, "Allocation failed for block %d (size %zu bytes).\n", i, size);
            fclose(csv);
            return EXIT_FAILURE;
        }
        memset(blocks[i], '0' + (i % 10), size);
        log_event(csv, "Allocation", "t_malloc", size, blocks[i], opTime);
        printf("Allocated block %d of size %zu bytes at %p\n", i, size, blocks[i]);
    }
    for (int i = 0; i < NUM_BLOCKS; i++) {
        start = clock();
        t_free(blocks[i]);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        log_event(csv, "Deallocation", "t_free", (i + 1) * 10, blocks[i], opTime);
        printf("Freed block %d at %p\n", i, blocks[i]);
    }

    // Performance Test: Varying allocation sizes on a logarithmic scale.
    // Allocation sizes: 1, 10, 100, 1K, 10K, 100K, 1M, 8M.
    size_t sizes[] = {1, 10, 100, 1024, 10240, 102400, 1024000, 8000000};
    int numSizes = sizeof(sizes) / sizeof(sizes[0]);
    printf("Starting performance tests for allocation sizes...\n");
    for (int i = 0; i < numSizes; i++) {
        size_t size = sizes[i];
        start = clock();
        void *ptr = t_malloc(size);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        if (!ptr) {
            fprintf(stderr, "Performance test: Allocation of %zu bytes failed.\n", size);
            continue;
        }
        memset(ptr, 0, size);
        log_event(csv, "Allocation", "t_malloc", size, ptr, opTime);
        start = clock();
        t_free(ptr);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        log_event(csv, "Deallocation", "t_free", size, ptr, opTime);
        printf("Performance test: Allocated and freed %zu bytes in %.8f seconds\n", size, opTime);
    }

    // ---------------------------------
    // Random Deallocation Allocation Test
    // ---------------------------------
    #define NUM_RANDOM_BLOCKS 20
    void *randomBlocks[NUM_RANDOM_BLOCKS];
    printf("Random Test: Allocating %d blocks of increasing size...\n", NUM_RANDOM_BLOCKS);
    for (int i = 0; i < NUM_RANDOM_BLOCKS; i++) {
        size_t size = (i + 1) * 15;  // arbitrary sizes.
        start = clock();
        randomBlocks[i] = t_malloc(size);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        if (!randomBlocks[i]) {
            fprintf(stderr, "Random allocation test: Allocation failed for block %d (size %zu bytes).\n", i, size);
            fclose(csv);
            return EXIT_FAILURE;
        }
        memset(randomBlocks[i], 'a' + (i % 26), size);
        log_event(csv, "Allocation", "t_malloc", size, randomBlocks[i], opTime);
        printf("Allocated random block %d of size %zu bytes at %p\n", i, size, randomBlocks[i]);
    }
    // Shuffle the indices using Fisher–Yates.
    int indices[NUM_RANDOM_BLOCKS];
    for (int i = 0; i < NUM_RANDOM_BLOCKS; i++)
        indices[i] = i;
    for (int i = NUM_RANDOM_BLOCKS - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
    // Free the blocks in random order.
    for (int i = 0; i < NUM_RANDOM_BLOCKS; i++) {
        int idx = indices[i];
        start = clock();
        t_free(randomBlocks[idx]);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        size_t size = (idx + 1) * 15;
        log_event(csv, "Deallocation", "t_free", size, randomBlocks[idx], opTime);
        printf("Freed random block %d (index %d) at %p\n", i, idx, randomBlocks[idx]);
    }

    // -------------------------
    // Mixed Allocation Test (Random Allocation/Deallocation)
    // -------------------------
    #define NUM_MIXED_OPS 50
    void *mixedBlocks[NUM_MIXED_OPS];
    int mixedCount = 0;
    printf("Mixed Test: Starting %d random allocation/deallocation operations...\n", NUM_MIXED_OPS);
    for (int i = 0; i < NUM_MIXED_OPS; i++) {
        if (mixedCount == 0 || (rand() % 2 == 0 && mixedCount < NUM_MIXED_OPS)) {
            // Allocate a block with a random size between 1 and 100 bytes.
            size_t size = (rand() % 100) + 1;
            start = clock();
            void *p = t_malloc(size);
            end = clock();
            opTime = (double)(end - start) / CLOCKS_PER_SEC;
            if (!p) {
                fprintf(stderr, "Mixed test: Allocation of %zu bytes failed.\n", size);
                continue;
            }
            memset(p, 'X', size);
            mixedBlocks[mixedCount++] = p;
            log_event(csv, "Allocation", "t_malloc", size, p, opTime);
            printf("Mixed Test: Allocated %zu bytes at %p\n", size, p);
        } else {
            // Free a random block from those allocated.
            int idx = rand() % mixedCount;
            start = clock();
            t_free(mixedBlocks[idx]);
            end = clock();
            opTime = (double)(end - start) / CLOCKS_PER_SEC;
            log_event(csv, "Deallocation", "t_free", 0, mixedBlocks[idx], opTime);
            printf("Mixed Test: Freed block at %p\n", mixedBlocks[idx]);
            // Remove the freed block by replacing it with the last element.
            mixedBlocks[idx] = mixedBlocks[--mixedCount];
        }
    }
    // Free any remaining blocks.
    for (int i = 0; i < mixedCount; i++) {
        start = clock();
        t_free(mixedBlocks[i]);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        log_event(csv, "Deallocation", "t_free", 0, mixedBlocks[i], opTime);
        printf("Mixed Test: Freed remaining block at %p\n", mixedBlocks[i]);
    }

    printf("Memory allocation tests completed successfully.\n");
    fclose(csv);
    return EXIT_SUCCESS;
}
