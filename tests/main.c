#include <stdio.h>
#include <stdlib.h>
#include <check.h>

Suite* lru_suite();
Suite* slab_suite();
Suite* hashmap_suite();
Suite* cache_suite();

int main (void) {
  int number_failed;
  
  SRunner *sr = srunner_create(lru_suite());
  srunner_add_suite(sr, slab_suite());
  srunner_add_suite(sr, hashmap_suite());
  srunner_add_suite(sr, cache_suite());
  
  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
