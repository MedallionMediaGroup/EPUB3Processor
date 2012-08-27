#include <config.h>
#include <check.h>
#include "test_common.h"
#include "EPUB3.h"
#include "EPUB3_private.h"

static EPUB3Ref epub;

static void setup()
{
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 2387538);
  epub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(epub, path);
}

static void teardown()
{
  EPUB3Release(epub);
}

START_TEST(test_epub3_get_file_count_in_archive)
{
  u_long expectedCount = 115U;
  u_long count = EPUB3GetFileCountInArchive(epub);
  fail_unless(count == expectedCount, "Expected %u files, but found %u in %s.", expectedCount, count, epub->archivePath);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 182);
  
  EPUB3Ref badMetadataEpub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(badMetadataEpub, path);
  count = EPUB3GetFileCountInArchive(badMetadataEpub);
  fail_unless(count == 1,  "Expected %u files, but found %u in %s.", 1, count, badMetadataEpub->archivePath);
  EPUB3Release(badMetadataEpub);
}
END_TEST

START_TEST(test_epub3_get_file_size_in_archive)
{
  const char * filename = "META-INF/container.xml";
  uint32_t expectedSize = 250U;
  uint32_t size;
  EPUB3Error error = EPUB3GetUncompressedSizeOfFileInArchive(epub, &size, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "Expected, but couldn't find %s in %s.", filename, epub->archivePath);
  fail_unless(error == kEPUB3Success, "Something went wrong when looking for %s in %s.", filename, epub->archivePath);
  fail_unless(size == expectedSize, "Expected size of %u, but got %u for %s.", expectedSize, size, filename);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 182);
  EPUB3Ref badMetadataEpub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(badMetadataEpub, path);
  
  filename = "mimetype";
  expectedSize = 16U;
  error = EPUB3GetUncompressedSizeOfFileInArchive(badMetadataEpub, &size, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "Expected, but couldn't find %s in %s.", filename, badMetadataEpub->archivePath);
  fail_unless(error == kEPUB3Success, "Something went wrong when looking for %s in %s.", filename, badMetadataEpub->archivePath);
  fail_unless(size == expectedSize, "Expected size of %u, but got %u for %s.", expectedSize, size, badMetadataEpub);
  EPUB3Release(badMetadataEpub);
  
  EPUB3Ref archiveless = EPUB3Create();
  error =  EPUB3GetUncompressedSizeOfFileInArchive(archiveless, &size, filename);
  fail_unless(error == kEPUB3ArchiveUnavailableError, "Function should not try to operate on an EPUB3Ref with an uninitialized archive.");
  EPUB3Release(archiveless);
}
END_TEST

START_TEST(test_epub3_validate_file_exists_in_zip)
{
  const char * filename = "META-INF/container.xml";
  EPUB3Error error = EPUB3ValidateFileExistsAndSeekInArchive(epub, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "File %s not found in %s.", filename, epub->archivePath);
  fail_unless(error == kEPUB3Success, "Had a problem looking for %s in %s.", filename, epub->archivePath);
  
  filename = "mimetype";
  error = EPUB3ValidateFileExistsAndSeekInArchive(epub, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "File %s not found in %s.", filename, epub->archivePath);
  fail_unless(error == kEPUB3Success, "Had a problem looking for %s in %s.", filename, epub->archivePath);

  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 182);
  EPUB3Ref badEpub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(badEpub, path);
  error = EPUB3ValidateFileExistsAndSeekInArchive(badEpub, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "File %s not found in %s.", filename, badEpub->archivePath);
  fail_unless(error == kEPUB3Success, "Had a problem looking for %s in %s.", filename, badEpub->archivePath);
  EPUB3Release(badEpub);
  
  EPUB3Ref archiveless = EPUB3Create();
  error =  EPUB3ValidateFileExistsAndSeekInArchive(archiveless, filename);
  fail_unless(error == kEPUB3ArchiveUnavailableError, "Function should not try to operate on an EPUB3Ref with an uninitialized archive.");
  EPUB3Release(archiveless);
}
END_TEST

START_TEST(test_epub3_copy_file_into_buffer)
{
  void *buffer = NULL;
  uint32_t bufferSize;
  uint32_t bytesCopied;
  const char * filename = "META-INF/container.xml";
  uint32_t expectedSize = 250U;
  EPUB3Error error = EPUB3CopyFileIntoBuffer(epub, &buffer, &bufferSize, &bytesCopied, filename);
  fail_unless(error == kEPUB3Success, "Copy into buffer failed with error: %d", error);
  fail_unless(bufferSize == expectedSize, "Expected %s to be %u bytes but was %u bytes.", filename, expectedSize, bufferSize);
  fail_unless(bytesCopied == expectedSize, "Expected %s to be %u bytes but was %u bytes.", filename, expectedSize, bufferSize);
  fail_unless(bytesCopied == bufferSize, "Expected %s to be %u bytes but was %u bytes.", filename, expectedSize, bufferSize);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "pg100_container.xml");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 250);
  FILE *containerFP = fopen(path, "r");
  char *newBuf = (char *)calloc(bufferSize, sizeof(char));
  size_t bytesRead = fread(newBuf, sizeof(char), bufferSize, containerFP);
  fail_if(ferror(containerFP) != 0, "Problem reading test data file %s: %s", path, strerror(ferror(containerFP)));
  fail_unless(feof(containerFP) == 0, "The test data file %s is bigger than the archive's file.");
  fail_unless(bytesRead == bufferSize, "The test data file %s is bigger than the archive's file.");
  fail_unless(strcmp(newBuf, buffer) == 0, "%s does not match the test data in %s.", filename, path);
  
  fclose(containerFP);
  free(newBuf);
  free(buffer);
  
  EPUB3Ref archiveless = EPUB3Create();
  error = EPUB3CopyFileIntoBuffer(archiveless, &buffer, &bufferSize, &bytesCopied, filename);
  fail_unless(error == kEPUB3ArchiveUnavailableError, "Function should not try to operate on an EPUB3Ref with an uninitialized archive.");
  EPUB3Release(archiveless);
}
END_TEST

START_TEST(test_epub3_copy_root_file_path_from_container)
{
  char *rootPath = NULL;
  const char * expectedPath = "100/content.opf";
  EPUB3Error error = EPUB3CopyRootFilePathFromContainer(epub, &rootPath);
  fail_unless(error == kEPUB3Success, "Error %d when trying to retrieve the root file path.", error);
  ck_assert_str_eq(rootPath, expectedPath);
  free(rootPath);
  
  EPUB3Ref archiveless = EPUB3Create();
  error = EPUB3CopyRootFilePathFromContainer(archiveless, &rootPath);
  fail_unless(error == kEPUB3ArchiveUnavailableError, "Function should not try to operate on an EPUB3Ref with an uninitialized archive.");
  EPUB3Release(archiveless);
}
END_TEST

START_TEST(test_epub3_parse_metadata_from_shakespeare_opf_data)
{
  const char * expectedTitle = "The Complete Works of William Shakespeare";
  const char * expectedIdentifier = "http://www.gutenberg.org/ebooks/100";
  const char * expectedLanguage = "en";

  TEST_PATH_VAR_FOR_FILENAME(path, "pg_100_content.opf");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 24804);
  EPUB3Ref blankEPUB = EPUB3Create();

  EPUB3MetadataRef blankMetadata = EPUB3MetadataCreate();
  EPUB3SetMetadata(blankEPUB, blankMetadata);
  
  EPUB3ManifestRef blankManifest = EPUB3ManifestCreate();
  EPUB3SetManifest(blankEPUB, blankManifest);
  
  EPUB3SpineRef blankSpine = EPUB3SpineCreate();
  EPUB3SetSpine(blankEPUB, blankSpine);
  
  struct stat st;
  stat(path, &st);
  off_t bufferSize = st.st_size;
  FILE *fp = fopen(path, "r");
  char *newBuf = (char *)calloc(bufferSize, sizeof(char));
  size_t bytesRead = fread(newBuf, sizeof(char), bufferSize, fp);
  
  fail_if(ferror(fp) != 0, "Problem reading test data file %s: %s", path, strerror(ferror(fp)));
  fail_unless(bytesRead == bufferSize, "Only read %d bytes of the %d byte test data file.", bytesRead, bufferSize);

  EPUB3Error error = EPUB3ParseFromOPFData(blankEPUB, newBuf, (uint32_t)bufferSize);
  fail_unless(error == kEPUB3Success);
  fail_if(blankEPUB->metadata == NULL);
  assert(blankEPUB->metadata != NULL);
  
  fail_if(blankEPUB->metadata->title == NULL, "A title is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->title != NULL);
  ck_assert_str_eq(blankEPUB->metadata->title, expectedTitle);

  fail_if(blankEPUB->metadata->identifier == NULL, "An identifier is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->identifier != NULL);
  ck_assert_str_eq(blankEPUB->metadata->identifier, expectedIdentifier);

  fail_if(blankEPUB->metadata->language == NULL, "A language is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->language != NULL);
  ck_assert_str_eq(blankEPUB->metadata->language, expectedLanguage);

  free(newBuf);
  EPUB3MetadataRelease(blankMetadata);
  EPUB3ManifestRelease(blankManifest);
  EPUB3SpineRelease(blankSpine);
  EPUB3Release(blankEPUB);
}
END_TEST

START_TEST(test_epub3_parse_metadata_from_moby_dick_opf_data)
{
  const char * expectedTitle = "Moby-Dick";
  const char * expectedIdentifier = "urn:isbn:9780316000000";
  const char * expectedLanguage = "en-US";

  TEST_PATH_VAR_FOR_FILENAME(path, "moby_dick_package.opf");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 23250);
  EPUB3Ref blankEPUB = EPUB3Create();

  EPUB3MetadataRef blankMetadata = EPUB3MetadataCreate();
  EPUB3SetMetadata(blankEPUB, blankMetadata);
  
  EPUB3ManifestRef blankManifest = EPUB3ManifestCreate();
  EPUB3SetManifest(blankEPUB, blankManifest);
  
  EPUB3SpineRef blankSpine = EPUB3SpineCreate();
  EPUB3SetSpine(blankEPUB, blankSpine);
  
  struct stat st;
  stat(path, &st);
  off_t bufferSize = st.st_size;
  FILE *fp = fopen(path, "r");
  char *newBuf = (char *)calloc(bufferSize, sizeof(char));
  size_t bytesRead = fread(newBuf, sizeof(char), bufferSize, fp);
  
  fail_if(ferror(fp) != 0, "Problem reading test data file %s: %s", path, strerror(ferror(fp)));
  fail_unless(bytesRead == bufferSize, "Only read %d bytes of the %d byte test data file.", bytesRead, bufferSize);

  EPUB3Error error = EPUB3ParseFromOPFData(blankEPUB, newBuf, (uint32_t)bufferSize);
  fail_unless(error == kEPUB3Success);
  fail_if(blankEPUB->metadata == NULL);
  assert(blankEPUB->metadata != NULL);
  
  fail_if(blankEPUB->metadata->title == NULL, "A title is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->title != NULL);
  ck_assert_str_eq(blankEPUB->metadata->title, expectedTitle);

  fail_if(blankEPUB->metadata->identifier == NULL, "An identifier is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->identifier != NULL);
  ck_assert_str_eq(blankEPUB->metadata->identifier, expectedIdentifier);

  fail_if(blankEPUB->metadata->language == NULL, "A language is required by the EPUB 3 spec.");
  assert(blankEPUB->metadata->language != NULL);
  ck_assert_str_eq(blankEPUB->metadata->language, expectedLanguage);

  free(newBuf);
  EPUB3MetadataRelease(blankMetadata);
  EPUB3ManifestRelease(blankManifest);
  EPUB3SpineRelease(blankSpine);
  EPUB3Release(blankEPUB);
}
END_TEST

START_TEST(test_epub3_parse_data_from_opf_using_real_epub)
{
  const char * expectedTitle = "The Complete Works of William Shakespeare";
  const char * expectedIdentifier = "http://www.gutenberg.org/ebooks/100";
  const char * expectedLanguage = "en";
  
  const char * expItem1Id = "id00000";
  const char * expItem1Href = "@public@vhost@g@gutenberg@html@dirs@etext94@shaks12-0.txt.html";
  const char * expItem1MediaType = "application/xhtml+xml";
  
  const char * expItem2Id = "id00720";
  const char * expItem2Href = "@public@vhost@g@gutenberg@html@dirs@etext94@shaks12-10.txt.html";
  const char * expItem2MediaType = "application/xhtml+xml";


  EPUB3Error error = EPUB3InitFromOPF(epub, "100/content.opf");
  fail_unless(error == kEPUB3Success);
  fail_if(epub->metadata == NULL);
  
  fail_if(epub->metadata->title == NULL, "A title is required by the EPUB 3 spec.");
  ck_assert_str_eq(epub->metadata->title, expectedTitle);

  fail_if(epub->metadata->identifier == NULL, "An identifier is required by the EPUB 3 spec.");
  ck_assert_str_eq(epub->metadata->identifier, expectedIdentifier);

  fail_if(epub->metadata->language == NULL, "A language is required by the EPUB 3 spec.");
  ck_assert_str_eq(epub->metadata->language, expectedLanguage);
  
  EPUB3ManifestItemRef item = EPUB3ManifestCopyItemWithId(epub->manifest, expItem1Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem1Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem1Id);
  ck_assert_str_eq(item->href, expItem1Href);
  ck_assert_str_eq(item->mediaType, expItem1MediaType);
  EPUB3ManifestItemRelease(item);
  
  item = EPUB3ManifestCopyItemWithId(epub->manifest, expItem2Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem2Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem2Id);
  ck_assert_str_eq(item->href, expItem2Href);
  ck_assert_str_eq(item->mediaType, expItem2MediaType);
  EPUB3ManifestItemRelease(item);
}
END_TEST

START_TEST(test_epub3_parse_manifest_from_shakespeare_opf_data)
{
  int32_t expectedManifestItemCount = 112;
  
  const char * expItem1Id = "id00000";
  const char * expItem1Href = "@public@vhost@g@gutenberg@html@dirs@etext94@shaks12-0.txt.html";
  const char * expItem1MediaType = "application/xhtml+xml";
  
  const char * expItem2Id = "id00720";
  const char * expItem2Href = "@public@vhost@g@gutenberg@html@dirs@etext94@shaks12-10.txt.html";
  const char * expItem2MediaType = "application/xhtml+xml";
  
  TEST_PATH_VAR_FOR_FILENAME(path, "pg_100_content.opf");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 24804);
  EPUB3Ref blankEPUB = EPUB3Create();
  
  EPUB3MetadataRef blankMetadata = EPUB3MetadataCreate();
  EPUB3SetMetadata(blankEPUB, blankMetadata);
  
  EPUB3ManifestRef blankManifest = EPUB3ManifestCreate();
  EPUB3SetManifest(blankEPUB, blankManifest);
  
  EPUB3SpineRef blankSpine = EPUB3SpineCreate();
  EPUB3SetSpine(blankEPUB, blankSpine);
  
  struct stat st;
  stat(path, &st);
  off_t bufferSize = st.st_size;
  FILE *fp = fopen(path, "r");
  char *newBuf = (char *)calloc(bufferSize, sizeof(char));
  size_t bytesRead = fread(newBuf, sizeof(char), bufferSize, fp);
  
  fail_if(ferror(fp) != 0, "Problem reading test data file %s: %s", path, strerror(ferror(fp)));
  fail_unless(bytesRead == bufferSize, "Only read %d bytes of the %d byte test data file.", bytesRead, bufferSize);
  
  EPUB3Error error = EPUB3ParseFromOPFData(blankEPUB, newBuf, (uint32_t)bufferSize);
  fail_unless(error == kEPUB3Success);
  fail_if(blankEPUB->manifest == NULL);
  
  ck_assert_int_eq(blankManifest->itemCount, expectedManifestItemCount);
  
  EPUB3ManifestItemRef item = EPUB3ManifestCopyItemWithId(blankManifest, expItem1Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem1Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem1Id);
  ck_assert_str_eq(item->href, expItem1Href);
  ck_assert_str_eq(item->mediaType, expItem1MediaType);
  EPUB3ManifestItemRelease(item);

  item = EPUB3ManifestCopyItemWithId(blankManifest, expItem2Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem2Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem2Id);
  ck_assert_str_eq(item->href, expItem2Href);
  ck_assert_str_eq(item->mediaType, expItem2MediaType);
  EPUB3ManifestItemRelease(item);
  
  free(newBuf);
  EPUB3MetadataRelease(blankMetadata);
  EPUB3ManifestRelease(blankManifest);
  EPUB3SpineRelease(blankSpine);
  EPUB3Release(blankEPUB);
}
END_TEST

START_TEST(test_epub3_parse_manifest_from_moby_dick_opf_data)
{
  int32_t expectedManifestItemCount = 155;
  
  const char * expItem1Id = "xchapter_013";
  const char * expItem1Href = "chapter_013.xhtml";
  const char * expItem1MediaType = "application/xhtml+xml";
  
  const char * expItem2Id = "titlepage";
  const char * expItem2Href = "titlepage.xhtml";
  const char * expItem2MediaType = "application/xhtml+xml";
  
  TEST_PATH_VAR_FOR_FILENAME(path, "moby_dick_package.opf");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 23250);
  EPUB3Ref blankEPUB = EPUB3Create();
  
  EPUB3MetadataRef blankMetadata = EPUB3MetadataCreate();
  EPUB3SetMetadata(blankEPUB, blankMetadata);
  
  EPUB3ManifestRef blankManifest = EPUB3ManifestCreate();
  EPUB3SetManifest(blankEPUB, blankManifest);
  
  EPUB3SpineRef blankSpine = EPUB3SpineCreate();
  EPUB3SetSpine(blankEPUB, blankSpine);
  
  struct stat st;
  stat(path, &st);
  off_t bufferSize = st.st_size;
  FILE *fp = fopen(path, "r");
  char *newBuf = (char *)calloc(bufferSize, sizeof(char));
  size_t bytesRead = fread(newBuf, sizeof(char), bufferSize, fp);
  
  fail_if(ferror(fp) != 0, "Problem reading test data file %s: %s", path, strerror(ferror(fp)));
  fail_unless(bytesRead == bufferSize, "Only read %d bytes of the %d byte test data file.", bytesRead, bufferSize);
  
  EPUB3Error error = EPUB3ParseFromOPFData(blankEPUB, newBuf, (uint32_t)bufferSize);
  fail_unless(error == kEPUB3Success);
  fail_if(blankEPUB->manifest == NULL);
  
  ck_assert_int_eq(blankManifest->itemCount, expectedManifestItemCount);
  
  EPUB3ManifestItemRef item = EPUB3ManifestCopyItemWithId(blankManifest, expItem1Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem1Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem1Id);
  ck_assert_str_eq(item->href, expItem1Href);
  ck_assert_str_eq(item->mediaType, expItem1MediaType);
  EPUB3ManifestItemRelease(item);
  
  item = EPUB3ManifestCopyItemWithId(blankManifest, expItem2Id);
  fail_if(item == NULL, "Could not fetch item with itemId %s from the manifest.", expItem2Id);
  assert(item != NULL);
  ck_assert_str_eq(item->itemId, expItem2Id);
  ck_assert_str_eq(item->href, expItem2Href);
  ck_assert_str_eq(item->mediaType, expItem2MediaType);
  EPUB3ManifestItemRelease(item);
  
  free(newBuf);
  EPUB3MetadataRelease(blankMetadata);
  EPUB3ManifestRelease(blankManifest);
  EPUB3SpineRelease(blankSpine);
  EPUB3Release(blankEPUB);
}
END_TEST

// Validation tests

START_TEST(test_epub3_validate_mimetype)
{
  EPUB3Error error = EPUB3ValidateMimetype(epub);
  fail_unless(error == kEPUB3Success);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  TEST_DATA_FILE_SIZE_SANITY_CHECK(path, 182);
  EPUB3Ref badEpub = EPUB3Create();
  (void)EPUB3PrepareArchiveAtPath(badEpub, path);
  error = EPUB3ValidateMimetype(badEpub);
  fail_if(error == kEPUB3Success);
  EPUB3Release(badEpub);
  
  EPUB3Ref archiveless = EPUB3Create();
  error =  EPUB3ValidateMimetype(archiveless);
  fail_unless(error == kEPUB3ArchiveUnavailableError, "Function should not try to operate on an EPUB3Ref with an uninitialized archive.");
  EPUB3Release(archiveless);
}
END_TEST


TEST_EXPORT TCase * check_EPUB3_parsing_make_tcase(void)
{
  TCase *test_case = tcase_create("EPUB3");
  tcase_add_checked_fixture(test_case, setup, teardown);
  tcase_add_test(test_case, test_epub3_get_file_count_in_archive);
  tcase_add_test(test_case, test_epub3_get_file_size_in_archive);
  tcase_add_test(test_case, test_epub3_validate_file_exists_in_zip);
  tcase_add_test(test_case, test_epub3_copy_file_into_buffer);
  tcase_add_test(test_case, test_epub3_parse_metadata_from_shakespeare_opf_data);
  tcase_add_test(test_case, test_epub3_parse_metadata_from_moby_dick_opf_data);
  tcase_add_test(test_case, test_epub3_parse_data_from_opf_using_real_epub);
  tcase_add_test(test_case, test_epub3_parse_manifest_from_shakespeare_opf_data);
  tcase_add_test(test_case, test_epub3_parse_manifest_from_moby_dick_opf_data);
  tcase_add_test(test_case, test_epub3_copy_root_file_path_from_container);
  tcase_add_test(test_case, test_epub3_validate_mimetype);
  return test_case;
}
