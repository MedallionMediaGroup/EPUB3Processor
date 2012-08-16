#include <config.h>
#include <check.h>
#include "EPUB3.h"
#include "EPUB3_private.h"

#ifndef TEST_DATA_PATH
#define TEST_DATA_PATH
#endif

START_TEST(test_create_epub3_object)
{
  EPUB3Ref epub = EPUB3Create();
  ck_assert_int_eq(epub->_type.refCount, 1);
  ck_assert_str_eq(epub->_type.typeID, kEPUB3TypeID);
}
END_TEST

TEST_EXPORT TCase * check_EPUB3_make_tcase(void)
{
  TCase *test_case = tcase_create("EPUB3");
  tcase_add_test(test_case, test_create_epub3_object);
  return test_case;
}
