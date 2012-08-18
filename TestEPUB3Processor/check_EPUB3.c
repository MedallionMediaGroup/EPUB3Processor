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

START_TEST(test_epub3_object_creation)
{
  fail_if(epub->archive == NULL);
  fail_unless(epub->archiveFileCount > 0);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  ck_assert_str_eq(epub->archivePath, path);
  fail_if(epub->archivePath == path);
}
END_TEST

START_TEST(test_epub3_object_ref_counting)
{
  EPUB3Ref anEpub = EPUB3Create();
  ck_assert_int_eq(anEpub->_type.refCount, 1);
  ck_assert_str_eq(anEpub->_type.typeID, kEPUB3TypeID);
  EPUB3Retain(anEpub);
  ck_assert_int_eq(anEpub->_type.refCount, 2);
  EPUB3Release(anEpub);
  ck_assert_int_eq(anEpub->_type.refCount, 1);
}
END_TEST

START_TEST(test_epub3_object_metadata_property)
{
  fail_unless(epub->metadata == NULL);
  EPUB3MetadataRef nullMeta = EPUB3CopyMetadata(epub);
  fail_unless(nullMeta == NULL);
  
  const char * title = "A book";
  EPUB3MetadataRef meta = EPUB3MetadataCreate();
  EPUB3MetadataSetTitle(meta, title);
  EPUB3SetMetadata(epub, meta);
  fail_unless(epub->metadata == meta);
  
  EPUB3MetadataRef metaCopy = EPUB3CopyMetadata(epub);
  ck_assert_str_eq(metaCopy->title, title);
  fail_if(metaCopy == meta);
  
  EPUB3MetadataRelease(metaCopy);
}
END_TEST

START_TEST(test_metadata_object)
{
  // Creation
  EPUB3MetadataRef meta = EPUB3MetadataCreate();
  ck_assert_int_eq(meta->_type.refCount, 1);
  ck_assert_str_eq(meta->_type.typeID, kEPUB3MetadataTypeID);
  fail_unless(meta->title == NULL);
  
  // Title attribute
  const char * title = "A book";
  EPUB3MetadataSetTitle(meta, title);
  ck_assert_str_eq(title, meta->title);
  fail_if(title == meta->title);
  char * titleCopy = EPUB3CopyMetadataTitle(meta);
  ck_assert_str_eq(title, titleCopy);
  fail_if(title == titleCopy);
  free(titleCopy);
}
END_TEST

TEST_EXPORT TCase * check_EPUB3_make_tcase(void)
{
  TCase *test_case = tcase_create("EPUB3");
  tcase_add_checked_fixture(test_case, setup, teardown);
  tcase_add_test(test_case, test_epub3_object_creation);
  tcase_add_test(test_case, test_epub3_object_ref_counting);
  tcase_add_test(test_case, test_epub3_object_metadata_property);
  tcase_add_test(test_case, test_metadata_object);
  return test_case;
}
