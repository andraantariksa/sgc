#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "sgc.h"

// Tag
#define SGC_TAG_NONE ((uint8_t)1 << (uint8_t)0)
#define SGC_TAG_MARKED ((uint8_t)1 << (uint8_t)1)

static sgc_t *sgc;

static sgc_alloc_t* sgc_alloc_new(void *ptr, size_t size, void (*destructor)(void*))
{
    sgc_alloc_t *alloc_map = (sgc_alloc_t*) malloc(sizeof(sgc_alloc_t));
    alloc_map->next = NULL;
    alloc_map->ptr = ptr;
    alloc_map->size = size;
    alloc_map->destructor = destructor;
    alloc_map->tag = SGC_TAG_NONE;
    return alloc_map;
}

static size_t sgc_alloc_map_hash(void *ptr)
{
    return ((uintptr_t)ptr) >> ((size_t)3);
}

static void sgc_alloc_map_delete(struct sgc_alloc_map_t* ptr)
{
    free(ptr);
}

void sgc_start(void *bottom_of_stack)
{
    sgc = (sgc_t*) malloc(sizeof(sgc_t));
    sgc->bottom_of_stack = bottom_of_stack;
    sgc->paused = false;
    sgc->alloc_map = (sgc_alloc_t**) calloc(sgc->capacity, sizeof(sgc_alloc_t*));
}

void sgc_run()
{
}

void sgc_malloc(size_t n)
{
    // Try to allocate
    void *mem = malloc(n);
    // Failed to allocate
    if (!mem)
    {
        // Try to run GC and free the memory
        sgc_run();
        // Allocate again
        mem = malloc(n);
        if (!mem)
        {
            // Ran out of memory
        }
    }
}

void sgc_free(void *ptr)
{
    free(ptr);
}

