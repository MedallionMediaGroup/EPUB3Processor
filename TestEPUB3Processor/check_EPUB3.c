#include <config.h>
#include <check.h>
#include <string.h>
#include "test_common.h"
#include "EPUB3.h"
#include "EPUB3_private.h"

static EPUB3Ref epub;
static char tmpDirname[22];

static void setup()
{
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 2376236);
  epub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(epub, path);
  strcpy(tmpDirname, "/tmp/epub3test-XXXXXX");
  fail_if(mkdtemp(tmpDirname) == NULL, "Unable to get a name for temp dir (%s) for test. (ERRNO %d)", tmpDirname, errno);
}

static void teardown()
{
  EPUB3Release(epub);
  fail_if(EPUB3RemoveDirectoryNamed(tmpDirname) < 0, "Problem removing %s after test. (ERRNO %d)", tmpDirname, errno);
}

#pragma mark -
#pragma mark test_epub3_object_creation
START_TEST(test_epub3_object_creation)
{
  fail_if(epub->archive == NULL);
  fail_unless(epub->archiveFileCount > 0);

  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 2376236);
  ck_assert_str_eq(epub->archivePath, path);
  fail_if(epub->archivePath == path);
}
END_TEST

#pragma mark test_epub3_object_ref_counting
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

#pragma mark test_epub3_object_metadata_property
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

#pragma mark test_metadata_object
START_TEST(test_metadata_object)
{
  // Creation
  EPUB3MetadataRef meta = EPUB3MetadataCreate();
  ck_assert_int_eq(meta->_type.refCount, 1);
  ck_assert_str_eq(meta->_type.typeID, kEPUB3MetadataTypeID);
  fail_unless(meta->title == NULL);

  EPUB3SetMetadata(epub, meta);

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

#pragma mark test_epub3_manifest_hash
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

#pragma mark test_epub3_spine
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

#pragma mark test_epub3_spine_list
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

#pragma mark test_epub3_copy_cover_image
START_TEST(test_epub3_copy_cover_image)
{
  fail_unless(EPUB3InitAndValidate(epub) == kEPUB3Success, "Unable to initialize and parse EPUB for testing.");
  TEST_PATH_VAR_FOR_FILENAME(testImagePath, "pg100_cover.jpg");
  struct stat st;
  int err = stat(testImagePath, &st);
  fail_unless(err == 0, "Error stat'ing the image at %s", testImagePath);
  int32_t testImageByteCount = (int32_t)st.st_size;

  void *testImageBytes = calloc(testImageByteCount, sizeof(char));
  FILE *testFile = fopen(testImagePath, "rb");
  size_t bytesRead = fread(testImageBytes, sizeof(char), testImageByteCount, testFile);
  fclose(testFile);
  ck_assert_int_eq(bytesRead, testImageByteCount);

  uint32_t testImageHash = SuperFastHash(testImageBytes, testImageByteCount);
  free(testImageBytes);

  void *bytes = NULL;
  uint32_t byteCount = 0;

  EPUB3Error error = EPUB3CopyCoverImage(epub, &bytes, &byteCount);
  fail_unless(error == kEPUB3Success);
  ck_assert_int_eq(byteCount, testImageByteCount);
  uint32_t actualImageHash = SuperFastHash(bytes, byteCount);
  ck_assert_int_eq(testImageHash, actualImageHash);

  free(bytes);

  EPUB3_FREE_AND_NULL(epub->metadata->coverImageId);
  error = EPUB3CopyCoverImage(epub, &bytes, &byteCount);
  fail_unless(error == kEPUB3FileNotFoundInArchiveError, "Should fail gracefully where there is no cover image.");
}
END_TEST

#pragma mark test_epub3_get_sequential_resource_paths
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

#pragma mark test_epub3_create_nested_directories
START_TEST(test_epub3_create_nested_directories)
{
  const char * filename = "a/lot/of/directories";
  uLong pathlen = strlen(tmpDirname) + 1U + strlen(filename) + 1U;
  char fullpath[pathlen];
  (void)strcpy(fullpath, tmpDirname);
  (void)strncat(fullpath, "/", 1U);
  (void)strncat(fullpath, filename, strlen(filename));

  EPUB3Error error = EPUB3CreateNestedDirectoriesForFileAtPath(fullpath);
  fail_unless(error == kEPUB3Success, "Could not create directory structure %s", fullpath);

  const char * dirname = "a/lot/of/";
  uLong dirlen = strlen(tmpDirname) + 1U + strlen(dirname) + 1U;
  char dirpath[dirlen];
  (void)strcpy(dirpath, tmpDirname);
  (void)strncat(dirpath, "/", 1U);
  (void)strncat(dirpath, dirname, strlen(dirname));

  struct stat st;
  fail_if(stat(dirpath, &st) < 0, "Directory (%s) was not created. (ERRNO %d)", dirpath, errno);
}
END_TEST

#pragma mark test_epub3_write_current_archive_file_to_path
START_TEST(test_epub3_write_current_archive_file_to_path)
{
  fail_unless(unzGoToFirstFile(epub->archive) == UNZ_OK, "Problem with zip file at path: %s", epub->archivePath);
  unz_file_info fileInfo;
  char filename[MAXNAMLEN];
  int err = unzGetCurrentFileInfo(epub->archive, &fileInfo, filename, MAXNAMLEN, NULL, 0, NULL, 0);
  fail_if(filename == NULL, "Unable to get name of current file in %s", epub->archivePath);
  fail_unless(err == UNZ_OK, "Problem reading info for file %s in %s", filename, epub->archivePath);

  (void)unzGoToFirstFile(epub->archive);
  EPUB3Error error = EPUB3WriteCurrentArchiveFileToPath(epub, tmpDirname);
  fail_unless(error == kEPUB3Success, "Couldn't write %s to %s.", filename, tmpDirname);

  uLong pathlen = strlen(tmpDirname) + 1U + strlen(filename) + 1U;
  char fullpath[pathlen];
  (void)strcpy(fullpath, tmpDirname);
  (void)strncat(fullpath, "/", 1U);
  (void)strncat(fullpath, filename, strlen(filename));

  struct stat st;
  err = stat(fullpath, &st);
  fail_if(err < 0, "File %s was not created.", filename);
  fail_unless(st.st_size == fileInfo.uncompressed_size);
}
END_TEST

#pragma mark test_epub3_extract_archive
START_TEST(test_epub3_extract_archive)
{
  char cwd[MAXNAMLEN];
  (void)getcwd(cwd, MAXNAMLEN);

  EPUB3Error error = EPUB3ExtractArchiveToPath(epub, tmpDirname);
  fail_unless(error == kEPUB3Success, "Unable to extract epub");

  const char * filename = "mimetype";
  uLong pathlen = strlen(tmpDirname) + 1U + strlen(filename) + 1U;
  char fullpath[pathlen];
  (void)strcpy(fullpath, tmpDirname);
  (void)strncat(fullpath, "/", 1U);
  (void)strncat(fullpath, filename, strlen(filename));

  struct stat st;
  int err = stat(fullpath, &st);
  fail_if(err < 0, "File %s was not extracted.", filename);

  const char * opffilename = "100/content.opf";
  uLong opfpathlen = strlen(tmpDirname) + 1U + strlen(opffilename) + 1U;
  char opfpath[opfpathlen];
  (void)strcpy(opfpath, tmpDirname);
  (void)strncat(opfpath, "/", 1U);
  (void)strncat(opfpath, opffilename, strlen(opffilename));

  err = stat(opfpath, &st);
  fail_if(err < 0, "File %s was not extracted.", opfpath);

  char cwd2[MAXNAMLEN];
  (void)getcwd(cwd2, MAXNAMLEN);
  ck_assert_str_eq(cwd, cwd2);
}
END_TEST

#pragma mark -
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
  tcase_add_test(test_case, test_epub3_copy_cover_image);
  tcase_add_test(test_case, test_epub3_get_sequential_resource_paths);
  tcase_add_test(test_case, test_epub3_write_current_archive_file_to_path);
  tcase_add_test(test_case, test_epub3_create_nested_directories);
  tcase_add_test(test_case, test_epub3_extract_archive);
  return test_case;
}
