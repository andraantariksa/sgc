#include <stdio.h>
#include <stdlib.h>
#include <sgc.h>
#include <munit.h>

sgc_gc_t *gc_instance;

int main(int argc, char **argv) {
  sgc_new(&argc);
  gc_instance = sgc_get_instance();

  munit_assert(sgc_is_prime(1031));
  munit_assert(!sgc_is_prime(1037));
  munit_assert(sgc_next_prime(1024) == 1031);

  int *a = (int *) sgc_malloc(sizeof(int), NULL);
  *a = 1;
  munit_assert(gc_instance->alloc_map->size == 1);
  sgc_alloc_t *a_alloc = gc_instance->alloc_map->allocs[sgc_alloc_map_hash(a) % gc_instance->alloc_map->capacity];
  munit_assert_ptr_equal((int *) a_alloc->ptr, a);
  munit_assert(a_alloc->tag == _SGC_TAG_NONE);
  munit_assert(a_alloc->tag & _SGC_TAG_NONE);
  munit_assert(a_alloc->size == 4);
  munit_assert_ptr_null(a_alloc->next);
  munit_assert_ptr_null(a_alloc->destructor);

  sgc_run();

  sgc_delete();
  gc_instance = sgc_get_instance();

  munit_assert_ptr_null(gc_instance);

  printf("Passed the test successfully\n");
}
