#include <config.h>
#include <check.h>
#include "test_common.h"
#include "EPUB3.h"
#include "EPUB3_private.h"

static EPUB3Ref epub;

static void setup()
{
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  epub = EPUB3CreateWithArchiveAtPath(path);
}

static void teardown()
{
  EPUB3Release(epub);
}

START_TEST(test_epub3_get_file_count_in_zip)
{
  u_long count = _GetFileCountInZipFile(epub->archive);
  fail_unless(count == 115U, "Expected %u files, but found %u in %s.", 115U, count, epub->archivePath);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  EPUB3Ref badMetadataEpub = EPUB3CreateWithArchiveAtPath(path);
  count = _GetFileCountInZipFile(badMetadataEpub->archive);
  fail_unless(count == 1,  "Expected %u files, but found %u in %s.", 115U, count, badMetadataEpub->archivePath);
  EPUB3Release(badMetadataEpub);
  
}
END_TEST

START_TEST(test_epub3_validate_mimetype)
{
  int result = EPUB3ValidateMimetype(epub);
  fail_unless(result == kEPUB3Success);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
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
  tcase_add_test(test_case, test_epub3_get_file_count_in_zip);
  tcase_add_test(test_case, test_epub3_validate_mimetype);
  return test_case;
}
