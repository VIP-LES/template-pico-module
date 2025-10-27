#include "o1heap.h"
#include "canard.h"

#define O1HEAP_ARENA_SIZE (16 * 1024)
static uint8_t o1heap_arena[O1HEAP_ARENA_SIZE];
static O1HeapInstance* o1_allocator = NULL;

O1HeapInstance* getAllocator(void) {
    if (o1_allocator == NULL) {
        o1_allocator = o1heapInit(o1heap_arena, sizeof(o1heap_arena));
    }
    return o1_allocator;
}

static void *memAllocate(void *const allocator, const size_t size) {
    (O1HeapInstance* )allocator;
    return o1heapAllocate(allocator, size);
}

static void memFree(void *const allocator, const size_t size, void *const pointer) {
    (O1HeapInstance*)allocator;
    (void)size;
    return o1heapFree(allocator, pointer);
}

#define TX_QUEUE_SIZE 100

void initCanard() {
    O1HeapInstance* allocator = getAllocator();
    if (allocator == NULL) {
        // handle allocator initialization failure
    }
    struct CanardMemoryResource memory = {allocator, memFree, memAllocate};
    struct CanardInstance canard = canardInit(memory);
    struct CanardTxQueue tx_queue = canardTxInit(
        TX_QUEUE_SIZE,
        CANARD_MTU_CAN_FD,
        memory
    );
}