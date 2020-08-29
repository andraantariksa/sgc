#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "sgc.h"

static sgc_gc_t *sgc_instance;

static sgc_alloc_t *sgc_alloc_new(void *ptr, size_t size, void (*destructor)(void *)) {
    sgc_alloc_t *alloc_map = (sgc_alloc_t *) malloc(sizeof(sgc_alloc_t));
    alloc_map->next = NULL;
    alloc_map->ptr = ptr;
    alloc_map->size = size;
    alloc_map->destructor = destructor;
    alloc_map->tag = _SGC_TAG_NONE;
    return alloc_map;
}

static bool sgc_is_prime(size_t n) {
    // https://stackoverflow.com/questions/1538644/c-determine-if-a-number-is-prime
    if (n <= 3)
        return n > 1;     // as 2 and 3 are prime
    else if (n % 2 == 0 || n % 3 == 0)
        return false;     // check if n is divisible by 2 or 3
    else {
        for (size_t i = 5; i * i <= n; i += 6) {
            if (n % i == 0 || n % (i + 2) == 0)
                return false;
        }
        return true;
    }
}

static size_t sgc_next_prime(size_t n) {
    if (!(n & (size_t) 0b1)) {
        n++; // Make it odd
    }

    while (!sgc_is_prime(n)) {
        n += 2;
    }
    return n;
}

static bool sgc_alloc_map_resize_to_fit(sgc_alloc_map_t *alloc_map) {
    double load_factor = sgc_alloc_map_load_factor(alloc_map);
    if (load_factor > alloc_map->upsize_factor) {
        sgc_alloc_map_resize(alloc_map, sgc_next_prime(alloc_map->capacity * 2));
        return true;
    }
    if (load_factor < alloc_map->downsize_factor) {
        sgc_alloc_map_resize(alloc_map, sgc_next_prime(alloc_map->capacity / 2));
        return true;
    }
    return false;
}

static size_t sgc_alloc_map_hash(void *alloc_ptr) {
    return ((uintptr_t) alloc_ptr) >> ((size_t) 3);
}

static void sgc_alloc_delete(sgc_alloc_t *alloc_ptr) {
    free(alloc_ptr);
}

static sgc_alloc_map_t *
sgc_alloc_map_new(size_t min_capacity, size_t capacity, double downsize_factor, double upsize_factor,
                  double sweep_factor) {
    sgc_alloc_map_t *alloc_map = (sgc_alloc_map_t *) malloc(sizeof(sgc_alloc_map_t));
    alloc_map->min_capacity = min_capacity;
    if (capacity < min_capacity) {
        capacity = min_capacity;
    }
    alloc_map->capacity = capacity;
    alloc_map->size = 0;
    alloc_map->downsize_factor = downsize_factor;
    alloc_map->upsize_factor = upsize_factor;
    alloc_map->sweep_factor = sweep_factor;
    alloc_map->allocs = (sgc_alloc_t **) calloc(capacity, sizeof(sgc_alloc_t *));
    return alloc_map;
}

static sgc_alloc_t *sgc_alloc_map_get(sgc_alloc_map_t *alloc_map, void *ptr) {
    size_t index = sgc_alloc_map_hash(ptr) % alloc_map->capacity;
    sgc_alloc_t *current_alloc = alloc_map->allocs[index];
    while (current_alloc) {
        if (current_alloc->ptr == ptr) {
            return current_alloc;
        }
        current_alloc = current_alloc->next;
    }
    return NULL;
}

static void sgc_alloc_map_remove(sgc_alloc_map_t *alloc_map, void *ptr, bool allow_resize) {
    size_t index = sgc_alloc_map_hash(ptr) % alloc_map->capacity;
    sgc_alloc_t *prev_alloc = NULL;
    sgc_alloc_t *current_alloc = alloc_map->allocs[index];
    while (current_alloc) {
        if (current_alloc->ptr == ptr) {
            // If found on the head
            if (!prev_alloc) {
                alloc_map->allocs[index] = current_alloc->next;
            }
                // Somewhere in the middle or the end
            else {
                prev_alloc->next = current_alloc->next;
            }
            sgc_alloc_delete(current_alloc);
            alloc_map->size--;
            break;
        } else {
            prev_alloc = current_alloc;
        }
        current_alloc = current_alloc->next;
    }
    if (allow_resize) {
        sgc_alloc_map_resize_to_fit(alloc_map);
    }
}

static void sgc_alloc_map_delete(sgc_alloc_map_t *alloc_map) {
    sgc_alloc_t *alloc, *tmp;
    for (size_t i = 0; i < alloc_map->size; ++i) {
        alloc = alloc_map->allocs[i];
        if (alloc) {
            while (alloc) {
                tmp = alloc;
                alloc = tmp->next;
                sgc_alloc_delete(tmp);
            }
        }
    }
    free(alloc_map->allocs);
    free(alloc_map);
}

static double sgc_alloc_map_load_factor(sgc_alloc_map_t *alloc_map_ptr) {
    return (double) alloc_map_ptr->size / (double) alloc_map_ptr->capacity;
}

static void sgc_alloc_map_resize(sgc_alloc_map_t *alloc_map, size_t new_capacity) {
    if (new_capacity < alloc_map->min_capacity) {
        return;
    }
    sgc_alloc_t **resized_allocs = calloc(new_capacity, sizeof(sgc_alloc_t *));

    for (size_t i = 0; i < alloc_map->capacity; ++i) {
        sgc_alloc_t *alloc = alloc_map->allocs[i];
        while (alloc) {
            sgc_alloc_t *next_alloc = alloc->next;

            size_t new_index = sgc_alloc_map_hash(alloc->ptr) % new_capacity;
            alloc->next = resized_allocs[new_index];
            resized_allocs[new_index] = alloc;

            alloc = next_alloc;
        }
    }

    free(alloc_map->allocs);
    alloc_map->capacity = new_capacity;
    alloc_map->allocs = resized_allocs;
//    alloc_map->sweep_limit
}

static void sgc_delete() {
    sgc_alloc_map_delete(sgc_instance->alloc_map);
    free(sgc_instance);
}

void sgc_new(void *bottom_of_stack) {
    sgc_new_ext(bottom_of_stack, _SGC_HASHMAP_DEFAULT_MIN_CAPACITY, _SGC_HASHMAP_DEFAULT_MIN_CAPACITY, 0.2, 0.8, 0.5);
}

void
sgc_new_ext(void *bottom_of_stack, size_t min_capacity, size_t capacity, double downsize_factor, double upsize_factor,
            double sweep_factor) {
    sgc_instance = (sgc_gc_t *) malloc(sizeof(sgc_gc_t));
    sgc_instance->bottom_of_stack = bottom_of_stack;
    sgc_instance->paused = false;
    sgc_instance->alloc_map = sgc_alloc_map_new(min_capacity, capacity, downsize_factor, upsize_factor, sweep_factor);

    atexit(sgc_delete);
}

void sgc_run() {
}

void sgc_malloc(size_t n) {
    // Try to allocate
    void *mem = malloc(n);
    // Failed to allocate
    if (!mem) {
        // Try to run GC and free the memory
        sgc_run();
        // Allocate again
        mem = malloc(n);
        if (!mem) {
            // Ran out of memory
        }
    }
}

static void sgc_mark_alloc(void *ptr) {
    sgc_alloc_t *alloc = sgc_alloc_map_get(sgc_instance->alloc_map, ptr);
    if (alloc) {
        // Is does not has a _SGC_TAG_MARKED flag
        if (!(alloc->tag & _SGC_TAG_MARKED)) {
            // Add _SGC_TAG_MARKED flag
            alloc->tag |= _SGC_TAG_MARKED;
//          TODO
//          Not sure
//            for (char* ptr_content = (char*)alloc->ptr; ptr_content <= (char*)alloc->ptr + alloc->size; ++ptr_content)
//            {
//                sgc_mark_alloc(*(void**)ptr);
//            }
        }
    }
}

static void sgc_mark_stack() {
    // TODO
    // GCC only
    void *top_of_stack = __builtin_frame_address(0);
    void *bottom_of_stack = sgc_instance->bottom_of_stack;
    // Add 1 bytes on each iteration
    for (char *ptr = (char *) top_of_stack; ptr <= (char *) bottom_of_stack - _PTRSIZE; ++ptr) {
        sgc_mark_alloc(*(void **) ptr);
    }
}

size_t gc_sweep() {
    size_t total = 0;
    for (size_t i = 0; i < sgc_instance->alloc_map->capacity; ++i) {
        sgc_alloc_t *chunk = sgc_instance->alloc_map->allocs[i];
        sgc_alloc_t *next = NULL;
        while (chunk) {
            if (chunk->tag & _SGC_TAG_MARKED) {
                // Remove marked tag
                chunk->tag &= (uint8_t) (~_SGC_TAG_MARKED);
                chunk = chunk->next;
            } else {
                total += chunk->size;
                if (chunk->destructor) {
                    chunk->destructor(chunk->ptr);
                }
                free(chunk->ptr);
                next = chunk->next;
                sgc_alloc_map_remove(sgc_instance->alloc_map, chunk->ptr, false);
                chunk = next;
            }
        }
    }
    sgc_alloc_map_resize_to_fit(sgc_instance->alloc_map);
}
