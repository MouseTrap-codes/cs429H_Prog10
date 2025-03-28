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

// Helper: Convert a string argument into an allocation strategy.
alloc_strat_e get_strategy(const char *str) {
    if (strcmp(str, "best") == 0)
        return BEST_FIT;
    else if (strcmp(str, "worst") == 0)
        return WORST_FIT;
    // Default to FIRST_FIT if unrecognized or "first" is provided.
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
void log_event(FILE *csv, const char *event, const char *operation, size_t size, void *ptr, double opTime) {
    size_t totalMemory = 0, allocatedMemory = 0;
    int blockCount = 0;
    get_memory_metrics(&totalMemory, &allocatedMemory, &blockCount);
    double utilization = (totalMemory > 0) ? ((double)allocatedMemory / totalMemory) * 100.0 : 0.0;
    
    // CSV columns: Event, Operation, BlockSize, Pointer, OperationTime(s), TotalMemory, AllocatedMemory, Utilization(%), BlockCount
    fprintf(csv, "%s,%s,%zu,%p,%.8f,%zu,%zu,%.2f,%d\n", event, operation, size, ptr, opTime, totalMemory, allocatedMemory, utilization, blockCount);
    fflush(csv);
}

int main(int argc, char *argv[]) {
    // Open CSV file for logging results.
    FILE *csv = fopen(CSV_FILENAME, "w");
    if (!csv) {
        perror("Failed to open CSV file for writing");
        return EXIT_FAILURE;
    }
    // Write CSV header (note the escaped %% for percent sign).
    fprintf(csv, "Event,Operation,BlockSize,Pointer,OpTime(s),TotalMemory,AllocatedMemory,Utilization(%%),BlockCount\n");

    // Determine allocation strategy from command-line argument.
    alloc_strat_e strategy = FIRST_FIT;
    if (argc > 1) {
        strategy = get_strategy(argv[1]);
    }

    // Display chosen strategy.
    printf("Initializing memory allocator with strategy: ");
    switch(strategy) {
        case FIRST_FIT: printf("FIRST_FIT\n"); break;
        case BEST_FIT:  printf("BEST_FIT\n");  break;
        case WORST_FIT: printf("WORST_FIT\n"); break;
        default:        printf("UNKNOWN (using FIRST_FIT)\n"); break;
    }

    // Record initialization event.
    clock_t initStart = clock();
    t_init(strategy);
    clock_t initEnd = clock();
    double initTime = (double)(initEnd - initStart) / CLOCKS_PER_SEC;
    log_event(csv, "Initialization", "t_init", 0, mmapRegion, initTime);

    // Test 1: Allocate 100 bytes.
    clock_t start = clock();
    void *p1 = t_malloc(100);
    clock_t end = clock();
    double opTime = (double)(end - start) / CLOCKS_PER_SEC;
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

    // Additional Test: Allocate and free multiple blocks.
    #define NUM_BLOCKS 10
    void *blocks[NUM_BLOCKS];
    printf("Allocating %d blocks of increasing size...\n", NUM_BLOCKS);
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
        // Optionally, write something into the allocated memory.
        memset(ptr, 0, size);
        log_event(csv, "Allocation", "t_malloc", size, ptr, opTime);
        // Immediately free the block.
        start = clock();
        t_free(ptr);
        end = clock();
        opTime = (double)(end - start) / CLOCKS_PER_SEC;
        log_event(csv, "Deallocation", "t_free", size, ptr, opTime);
        printf("Performance test: Allocated and freed %zu bytes in %.8f seconds\n", size, opTime);
    }

    // End of tests.
    printf("Memory allocation tests completed successfully.\n");
    fclose(csv);
    return EXIT_SUCCESS;
}
