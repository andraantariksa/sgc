#ifndef __SGC_H__
#define __SGC_H__

#include <stdbool.h>
#include <stdint.h>

// Types
struct sgc_alloc_map_t;

typedef struct {
    void* ptr;
    size_t size;
    uint8_t tag;
    void (*destructor)(void*);
    struct sgc_alloc_t *next;
} sgc_alloc_t;

typedef struct {
    size_t capacity;
    void *bottom_of_stack;
    bool paused;
    sgc_alloc_t** alloc_map;
} sgc_t;

// Main function to be used
void sgc_start(void *bottom_of_stack);
void sgc_run();
void sgc_malloc(size_t n);
void sgc_free(void *ptr);

// Internal function. Not intented to be used by the user
static sgc_alloc_t* sgc_alloc_new(void *ptr, size_t size, void (*destructor)(void*));
static size_t sgc_alloc_map_hash(void *ptr);
static void sgc_alloc_map_delete(struct sgc_alloc_map_t* ptr);

#endif // __SGC_H__
