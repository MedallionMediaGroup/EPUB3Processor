#include <config.h>
#include <check.h>
#include <string.h>
#include "test_common.h"
#include "EPUB3.h"
#include "EPUB3_private.h"

static EPUB3Ref epub;

static void setup()
{
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 2387538);
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
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 2387538);
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
  
  EPUB3SetMetadata(epub, meta);
  
  // Title attribute
  const char * title = "A book";
  const char * identifier = "myid";
  const char * language = "en";
  
  EPUB3MetadataSetTitle(meta, title);
  ck_assert_str_eq(title, meta->title);
  fail_if(title == meta->title);
  char * titleCopy = EPUB3CopyTitle(epub);
  ck_assert_str_eq(title, titleCopy);
  fail_if(title == titleCopy);
  free(titleCopy);

  EPUB3MetadataSetLanguage(meta, language);
  ck_assert_str_eq(language, meta->language);
  fail_if(language == meta->language);
  char * languageCopy = EPUB3CopyLanguage(epub);
  ck_assert_str_eq(language, languageCopy);
  fail_if(language == languageCopy);
  free(languageCopy);

  EPUB3MetadataSetIdentifier(meta, identifier);
  ck_assert_str_eq(identifier, meta->identifier);
  fail_if(identifier == meta->identifier);
  char * identifierCopy = EPUB3CopyIdentifier(epub);
  ck_assert_str_eq(identifier, identifierCopy);
  fail_if(identifier == identifierCopy);
  free(identifierCopy);
}
END_TEST

START_TEST(test_epub3_manifest_hash)
{
  EPUB3ManifestRef manifest = EPUB3ManifestCreate();
  ck_assert_int_eq(manifest->_type.refCount, 1);
  ck_assert_str_eq(manifest->_type.typeID, kEPUB3ManifestTypeID);
  fail_unless(manifest->itemCount == 0);

  EPUB3ManifestItemRef item = EPUB3ManifestItemCreate();
  ck_assert_int_eq(item->_type.refCount, 1);
  ck_assert_str_eq(item->_type.typeID, kEPUB3ManifestItemTypeID);
  const char * itemId = "myid";
  item->itemId = strdup(itemId);

  EPUB3ManifestInsertItem(manifest, item);
  fail_unless(manifest->itemCount == 1, "Incorrect item count in item hash table");

  EPUB3ManifestItemRef itemCopy = EPUB3ManifestCopyItemWithId(manifest, itemId);
  fail_if(itemCopy == NULL);
  assert(itemCopy != NULL);
  fail_if(itemCopy == item);
  ck_assert_str_eq(itemCopy->itemId, itemId);
  fail_if(itemCopy->itemId == itemId);
  ck_assert_str_eq(item->itemId, itemCopy->itemId);
  fail_if(item->itemId == itemCopy->itemId);
  
  itemCopy = EPUB3ManifestCopyItemWithId(manifest, "doesnotexist");
  fail_unless(itemCopy == NULL, "Non existent items in the manifest should be NULL.");

  EPUB3ManifestItemRelease(item);
  EPUB3ManifestRelease(manifest);
}
END_TEST

START_TEST(test_epub3_spine)
{
  EPUB3SpineRef spine = EPUB3SpineCreate();
  ck_assert_int_eq(spine->_type.refCount, 1);
  ck_assert_str_eq(spine->_type.typeID, kEPUB3SpineTypeID);
  fail_unless(spine->itemCount == 0);
  
  EPUB3SpineItemRef item = EPUB3SpineItemCreate();
  ck_assert_int_eq(item->_type.refCount, 1);
  ck_assert_str_eq(item->_type.typeID, kEPUB3SpineItemTypeID);
  
  EPUB3ManifestItemRef manifestItem = EPUB3ManifestItemCreate();
  const char * itemId = "myid";
  manifestItem->itemId = strdup(itemId);
  EPUB3SpineItemSetManifestItem(item, manifestItem);
  ck_assert_int_eq(manifestItem->_type.refCount, 1);
  EPUB3SpineItemRelease(item);
  ck_assert_int_eq(manifestItem->_type.refCount, 1);
  EPUB3ManifestItemRelease(manifestItem);
}
END_TEST

START_TEST(test_epub3_spine_list)
{
  EPUB3SpineRef spine = EPUB3SpineCreate();
  fail_unless(spine->itemCount == 0);

  int itemCount = 20;

  EPUB3SpineItemRef firstItem = EPUB3SpineItemCreate();
  
  fail_unless(spine->head == NULL);
  fail_unless(spine->tail == NULL);
  EPUB3SpineAppendItem(spine, firstItem);
  ck_assert_int_eq(firstItem->_type.refCount, 2);
  EPUB3SpineItemRelease(firstItem);
  ck_assert_int_eq(firstItem->_type.refCount, 1);
  fail_if(spine->head != spine->tail);
  fail_unless(spine->head->item == firstItem);
  fail_unless(spine->tail->item == firstItem);

  EPUB3SpineItemListItemPtr prev = spine->tail;
  
  for(int i = 1; i < itemCount; i++) {
    EPUB3SpineItemRef item = EPUB3SpineItemCreate();
    item->isLinear = i % 2;
    EPUB3SpineAppendItem(spine, item);
    
    fail_unless(spine->tail->item == item);
    prev = spine->tail;

    EPUB3SpineItemRelease(item);
  }
  
  ck_assert_int_eq(spine->itemCount, itemCount);
  fail_unless(spine->head->item == firstItem);
  fail_unless(spine->tail == prev);
  fail_if(spine->head == spine->tail);
  fail_unless(spine->tail->item->isLinear);
  EPUB3SpineRelease(spine);
}
END_TEST

START_TEST(test_epub3_get_sequential_resource_paths)
{
  EPUB3Error error = EPUB3InitAndValidate(epub);
  fail_unless(error == kEPUB3Success);
  int32_t expectedCount = 108;
  int32_t count = EPUB3CountOfSequentialResources(epub);
  ck_assert_int_eq(count, expectedCount);
  
  const char *resources[count];
  error = EPUB3GetPathsOfSequentialResources(epub, resources);
  fail_unless(error == kEPUB3Success);
  
  for (int i = 0; i < count; i++) {
    int maxPathlen = 65;
    char * expectedResourcePath = malloc(maxPathlen);
    snprintf(expectedResourcePath, maxPathlen, "@public@vhost@g@gutenberg@html@dirs@etext94@shaks12-%d.txt.html", i);
    ck_assert_str_eq(resources[i], expectedResourcePath);
    free(expectedResourcePath);
  }
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
  tcase_add_test(test_case, test_epub3_manifest_hash);
  tcase_add_test(test_case, test_epub3_spine);
  tcase_add_test(test_case, test_epub3_spine_list);
  tcase_add_test(test_case, test_epub3_get_sequential_resource_paths);
  return test_case;
}
