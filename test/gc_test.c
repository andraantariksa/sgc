#include <stdio.h>
#include <stdlib.h>
#include <sgc.h>

int main(int argc, char **argv) {
  sgc_new(&argc);
  int *a = (int *) sgc_malloc(sizeof(int), NULL);
  int *b = (int *) sgc_malloc(sizeof(int), NULL);
  int *c = (int *) sgc_malloc(sizeof(int), NULL);
  int *d = (int *) sgc_malloc(sizeof(int), NULL);
  int *e = (int *) sgc_malloc(sizeof(int), NULL);
  int *f = (int *) sgc_malloc(sizeof(int), NULL);
  int *g = (int *) sgc_malloc(sizeof(int), NULL);
  printf("All is well\n");
  printf("Stop the world\n");
  printf("Total allocated %zu\n", sgc_get_instance()->alloc_map->size);
  sgc_run();
  printf("Done\n");
}
