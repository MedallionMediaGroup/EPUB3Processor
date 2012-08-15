#include <config.h>
#include <check.h>

START_TEST(test_fail)
{
  fail_if(1==0, "You are not god!");
}
END_TEST

TEST_EXPORT TCase * check_EPUB3_make_tcase(void)
{
  TCase *test_case = tcase_create("EPUB3");
  tcase_add_test(test_case, test_fail);
  return test_case;
}
