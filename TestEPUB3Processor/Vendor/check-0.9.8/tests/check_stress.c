#include "../lib/libcompat.h"

/* note: this test appears pretty useless, so we aren't including it
   in the TESTS variable of Makefile.am */

#include <stdlib.h>
#include <stdio.h>
#include <check.h>


Suite *s;
TCase *tc;
SRunner *sr;

START_TEST(test_pass)
{
  fail_unless(1,"Shouldn't see this message");
}
END_TEST

START_TEST(test_fail)
{
  fail("This test fails");
}
END_TEST


static void run (int num_iters)
{
  int i;
  s = suite_create ("Stress");
  tc = tcase_create ("Stress");
  sr = srunner_create (s);
  suite_add_tcase(s, tc);

  for (i = 0; i < num_iters; i++) {
    tcase_add_test (tc, test_pass);
    tcase_add_test (tc, test_fail);
  }

  srunner_run_all(sr, CK_SILENT);
  if (srunner_ntests_failed (sr) != num_iters) {
    printf ("Error: expected %d failures, got %d\n",
	    num_iters, srunner_ntests_failed(sr));
    return;
  }

  srunner_free(sr);
}

  
int main(void)
{
  int i;
  time_t t1;
  int iters[] = {1, 100, 1000, 2000, 4000, 8000, 10000, 20000, 40000, -1};

  for (i = 0; iters[i] != -1; i++) {
    t1 = time(NULL);
    run(iters[i]);
    printf ("%d, %d\n", iters[i], (int) difftime(time(NULL), t1));
  }
  return 0;
}
