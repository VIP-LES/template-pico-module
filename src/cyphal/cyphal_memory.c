/**
 * @file cyphal_memory.c
 * @brief Deterministic memory allocator implementation using o1Heap for Libcanard.
 */

#include "cyphal_memory.h"
#include "o1heap.h"
#include <stdio.h>

// Heap size ≈ (# concurrent transfers) x (max payload size + ~100 B overhead)
// This value NEEDS TO BE TUNED!!!
#define CYPHAL_HEAP_SIZE 2048U

static uint8_t cyphal_heap_buffer[CYPHAL_HEAP_SIZE];
static O1HeapInstance* cyphal_heap = NULL;


void cyphal_memory_init(void) {
    cyphal_heap = o1heapInit(cyphal_heap_buffer, sizeof(cyphal_heap_buffer));

    if (cyphal_heap == NULL)
    {
        // Initialization failed
        printf("O1Heap init failed\n");
    }
}


// Private to this file – used as the memory allocation callback in a
// CanardMemoryResource.
static void* cyphal_allocate(void* const user_reference, const size_t amount) {
    (void)user_reference;
    return o1heapAllocate(cyphal_heap, amount);
}


// Private to this file – used as the memory deallocation callback in a
// CanardMemoryResource.
static void cyphal_deallocate(void* const user_reference,
                              const size_t amount,
                              void* const pointer) {
    (void)user_reference;
    (void)amount;  // O1Heap doesn’t need the size on free
    o1heapFree(cyphal_heap, pointer);
}


CanardMemoryResource cyphal_memory_resource(void) {
    CanardMemoryResource mem = {
        .user_reference = NULL,
        .allocate  = cyphal_allocate,
        .deallocate = cyphal_deallocate
    };
    return mem;
}

