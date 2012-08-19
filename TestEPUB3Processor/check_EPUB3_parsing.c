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
  u_long expectedCount = 115U;
  u_long count = _GetFileCountInZipFile(epub->archive);
  fail_unless(count == expectedCount, "Expected %u files, but found %u in %s.", expectedCount, count, epub->archivePath);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  EPUB3Ref badMetadataEpub = EPUB3CreateWithArchiveAtPath(path);
  count = _GetFileCountInZipFile(badMetadataEpub->archive);
  fail_unless(count == 1,  "Expected %u files, but found %u in %s.", 1, count, badMetadataEpub->archivePath);
  EPUB3Release(badMetadataEpub);
}
END_TEST

START_TEST(test_epub3_get_file_size_in_zip)
{
  const char * filename = "META-INF/container.xml";
  uint32_t expectedSize = 250U;
  uint32_t size;
  EPUB3Error error = EPUB3GetUncompressedSizeOfFileInArchive(epub, &size, filename);
  fail_if(error == kEPUB3FileNotFoundInArchiveError, "Expected, but couldn't find %s in %s.", filename, epub->archivePath);
  fail_unless(error == kEPUB3Success, "Something went wrong when looking for %s in %s.", filename, epub->archivePath);
  fail_unless(size == expectedSize, "Expected size of %u, but got %u for %s.", expectedSize, size, filename);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  EPUB3Ref badMetadataEpub = EPUB3CreateWithArchiveAtPath(path);
  
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
  EPUB3Ref badEpub = EPUB3CreateWithArchiveAtPath(path);
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

// Validation tests

START_TEST(test_epub3_validate_mimetype)
{
  EPUB3Error error = EPUB3ValidateMimetype(epub);
  fail_unless(error == kEPUB3Success);
  
  TEST_PATH_VAR_FOR_FILENAME(path, "bad_metadata.epub");
  EPUB3Ref badEpub = EPUB3CreateWithArchiveAtPath(path);
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
  tcase_add_test(test_case, test_epub3_get_file_count_in_zip);
  tcase_add_test(test_case, test_epub3_get_file_size_in_zip);
  tcase_add_test(test_case, test_epub3_validate_file_exists_in_zip);
  tcase_add_test(test_case, test_epub3_copy_file_into_buffer);
  tcase_add_test(test_case, test_epub3_copy_root_file_path_from_container);
  tcase_add_test(test_case, test_epub3_validate_mimetype);
  return test_case;
}
