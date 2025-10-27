/**
 * @file cyphal_memory.h
 * @brief Deterministic memory allocator interface for Libcanard v4 using o1Heap.
 *
 * This header provides a fixed-size memory allocator required by Libcanard.
 * It wraps the o1Heap allocator and exposes a CanardMemoryResource struct
 * that can be passed directly to canardInit().
 */

#ifndef CYPHAL_MEMORY_H
#define CYPHAL_MEMORY_H

#include "canard.h"
#include <stddef.h>

typedef struct CanardMemoryResource CanardMemoryResource;

/**
 * @brief Initializes the o1Heap allocator.
 *
 * Must be called once before creating the CanardInstance.
 */
void cyphal_memory_init(void);

/**
 * @brief Returns a CanardMemoryResource configured to use o1Heap.
 *
 * Example:
 * @code
 * cyphal_memory_init();
 * CanardMemoryResource mem = cyphal_memory_resource();
 * CanardInstance ins = canardInit(mem);
 * @endcode
 *
 * @return A fully configured CanardMemoryResource object.
 */
CanardMemoryResource cyphal_memory_resource(void);

#endif // CYPHAL_MEMORY_H