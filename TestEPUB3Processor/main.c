#include <stdio.h>
#include <config.h>
#include <check.h>
#include "test_common.h"

// Test suite prototypes
TCase * check_EPUB3_make_tcase();
TCase * check_EPUB3_parsing_make_tcase();

int main(int argc, const char * argv[])
{
  // insert code here...
  int number_failed;
  Suite *suite = suite_create("TestEPUB3Processor");
  
  suite_add_tcase(suite, check_EPUB3_make_tcase());
  suite_add_tcase(suite, check_EPUB3_parsing_make_tcase());
  
  SRunner *suite_runner = srunner_create(suite);
  
  if(argc > 1) {
    srunner_set_xml(suite_runner, argv[1]);
  }
  
  srunner_run_all(suite_runner, CK_NORMAL);
  number_failed = srunner_ntests_failed(suite_runner);
  srunner_free(suite_runner);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

