#include <config.h>
#include <check.h>
#include "test_common.h"
#include "EPUB3.h"
#include "EPUB3_private.h"

static EPUB3Ref epub;

static void setup()
{
  uint64_t length = EPUB3TestPathLengthForFileNamed("pg100.epub");
  char path[length];
  (void)EPUB3GetTestPathForFileNamed(path, "pg100.epub");
  epub = EPUB3CreateWithArchiveAtPath(path);
}

static void teardown()
{
  EPUB3Release(epub);
}

START_TEST(test_epub3_validate_mimetype)
{
  int result = EPUB3ValidateMimetype(epub);
  fail_unless(result == kEPUB3Success);
  
  uint64_t length = EPUB3TestPathLengthForFileNamed("bad_metadata.epub");
  char path[length];
  (void)EPUB3GetTestPathForFileNamed(path, "bad_metadata.epub");
  EPUB3Ref badEpub = EPUB3CreateWithArchiveAtPath(path);
  result = EPUB3ValidateMimetype(badEpub);
  fail_if(result == kEPUB3Success);
  EPUB3Release(badEpub);
}
END_TEST

TEST_EXPORT TCase * check_EPUB3_parsing_make_tcase(void)
{
  TCase *test_case = tcase_create("EPUB3");
  tcase_add_checked_fixture(test_case, setup, teardown);
  tcase_add_test(test_case, test_epub3_validate_mimetype);
  return test_case;
}
