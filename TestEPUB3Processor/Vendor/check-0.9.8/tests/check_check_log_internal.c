#include "../lib/libcompat.h"

/* Tests for log related stuff in check which need non-exported functions. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include <check_list.h>
#include <check_impl.h>
#include <check_log.h>
#include "check_check.h"


#if ENABLE_SUBUNIT
START_TEST(test_init_logging_subunit)
{
  /* init_logging with CK_SUBUNIT sets stdout 
   * to a subunit function, not any log.
   */
  Log * first_log = NULL;
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);
  srunner_init_logging(sr, CK_SUBUNIT);
  list_front (sr->loglst);
  fail_if (list_at_end(sr->loglst), "No entries in log list");
  first_log = list_val(sr->loglst);
  fail_if (first_log == NULL, "log is NULL");
  list_advance(sr->loglst);
  fail_unless(list_at_end(sr->loglst), "More than one entry in log list");
  fail_unless(first_log->lfun == subunit_lfun,
              "Log function is not the subunit lfun.");
  srunner_end_logging(sr);
  srunner_free(sr);
}
END_TEST
#endif

Suite *make_log_internal_suite(void)
{

  Suite *s;
  TCase *tc_core_subunit;

  s = suite_create("Log");
  tc_core_subunit = tcase_create("Core SubUnit");

#if ENABLE_SUBUNIT
  suite_add_tcase(s, tc_core_subunit);
  tcase_add_test(tc_core_subunit, test_init_logging_subunit);
#endif
  
  return s;
}

