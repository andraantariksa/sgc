#define _SGC_HASHMAP_DEFAULT_MIN_CAPACITY 7

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sgc.h>
#include <munit.h>

#define FORCE_LINKING(x) { void* volatile dummy = &x; }

sgc_gc_t *gc_instance;

void simple_alloc_test() {
  int *a = (int *) sgc_malloc(sizeof(int), NULL);
  *a = 1;

  munit_assert(gc_instance->alloc_map->size == 1);
  sgc_alloc_t *a_alloc = gc_instance->alloc_map->allocs[sgc_alloc_map_hash(a) % gc_instance->alloc_map->capacity];
  munit_assert_ptr_equal((int *) a_alloc->ptr, a);
  munit_assert(a_alloc->tag == _SGC_TAG_NONE);
  munit_assert(a_alloc->tag & _SGC_TAG_NONE);
  munit_assert(a_alloc->size == sizeof(int));
  munit_assert_ptr_null(a_alloc->next);
  munit_assert_ptr_null(a_alloc->destructor);

  sgc_alloc_map_remove(gc_instance->alloc_map, a, false);
  free(a);

  printf("prime_test - PASSED\n");
}

void prime_test() {
  munit_assert(sgc_is_prime(1031));
  munit_assert(!sgc_is_prime(1037));
  munit_assert(sgc_next_prime(1024) == 1031);
  printf("simple_alloc_test - PASSED\n");
}

void allocate_5() {
  int a = 5;
  while (a--) {
    int *ptr = sgc_malloc(sizeof(int), NULL);
    *ptr = a;
//    printf("%d address is %p\n", a, &ptr);
  }

  int *ptr = NULL;
  FORCE_LINKING(ptr); // Prevent compiler from removing the ptr for optimizing

  munit_assert(gc_instance->alloc_map->size == 5);
  sgc_run();
  munit_assert(gc_instance->alloc_map->size == 0);
}

int main(int argc, char **argv) {
  sgc_new(&argc);

  gc_instance = sgc_get_instance();

  prime_test();
  simple_alloc_test();
  allocate_5();

  sgc_delete();
  gc_instance = sgc_get_instance();

  munit_assert_ptr_null(gc_instance);

  printf("Passed the test successfully\n");
}
