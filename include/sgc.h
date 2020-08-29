#ifndef __SGC_H__
#define __SGC_H__

#include <stdbool.h>
#include <stdint.h>

/// Tag
#define _SGC_TAG_NONE (uint8_t)((uint8_t)1 << (uint8_t)0)
#define _SGC_TAG_MARKED (uint8_t)((uint8_t)1 << (uint8_t)1)

// https://primes.utm.edu/lists/small/10000.txt
#define _SGC_HASHMAP_DEFAULT_MIN_CAPACITY 1031

#define _PTRSIZE sizeof(char*)

/// Types
typedef struct sgc_alloc_t sgc_alloc_t;

struct sgc_alloc_t {
    void *ptr;
    size_t size;
    uint8_t tag;

    void (*destructor)(void *);

    sgc_alloc_t *next;
};

typedef struct {
    size_t capacity;
    size_t min_capacity;
    size_t size;
    double upsize_factor;
    double downsize_factor;
    double sweep_factor;
    sgc_alloc_t **allocs;
} sgc_alloc_map_t;

typedef struct {
    void *bottom_of_stack;
    bool paused;
    sgc_alloc_map_t *alloc_map;
} sgc_gc_t;

void sgc_new(void *bottom_of_stack);

void
sgc_new_ext(void *bottom_of_stack, size_t min_capacity, size_t capacity, double downsize_factor, double upsize_factor,
            double sweep_factor);

void sgc_run();

void sgc_malloc(size_t n);

size_t gc_sweep();

// Static

static sgc_alloc_t *sgc_alloc_new(void *ptr, size_t size, void (*destructor)(void *));

static bool sgc_alloc_map_resize_to_fit(sgc_alloc_map_t *alloc_map);

static size_t sgc_next_prime(size_t n);

static bool sgc_is_prime(size_t n);

static size_t sgc_alloc_map_hash(void *alloc_ptr);

static sgc_alloc_map_t *
sgc_alloc_map_new(size_t min_capacity, size_t capacity, double downsize_factor, double upsize_factor,
                  double sweep_factor);

static sgc_alloc_t *sgc_alloc_map_get(sgc_alloc_map_t *alloc_map, void *ptr);

static void sgc_alloc_map_remove(sgc_alloc_map_t *alloc_map, void *ptr, bool allow_resize);

static void sgc_alloc_map_delete(sgc_alloc_map_t *alloc_map);

static double sgc_alloc_map_load_factor(sgc_alloc_map_t *alloc_map_ptr);

static void sgc_alloc_map_resize(sgc_alloc_map_t *alloc_map, size_t new_capacity);

static void sgc_delete();

static void sgc_mark_alloc(void *ptr);

static void sgc_mark_stack()

#endif // __SGC_H__
