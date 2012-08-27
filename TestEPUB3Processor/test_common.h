#ifndef EPUB3Processor_test_common_h
#define EPUB3Processor_test_common_h

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define TEST_EXPORT __attribute__((visibility("default")))

#ifndef TEST_DATA_PATH
#define TEST_DATA_PATH "/tmp/"
#endif

#define TEST_PATH_VAR_FOR_FILENAME(__varname, __filename) \
  char __varname[EPUB3TestPathLengthForFileNamed(__filename)]; \
  (void)EPUB3GetTestPathForFileNamed(__varname, __filename);

#define TEST_DATA_FILE_SIZE_SANITY_CHECK(__path, __expectedSize) do {\
  struct stat st;\
  stat(__path, &st);\
  fail_unless(st.st_size == __expectedSize, "Incorrect file size (was %lld bytes; epxected %d bytes) for test data file: %s. (%s:%d)", st.st_size, __expectedSize, __path, __FILE__, __LINE__); \
} while(0);


static uint64_t EPUB3TestPathLengthForFileNamed(const char * name) {
  return strlen(TEST_DATA_PATH) + strlen(name) + 1;
}

static void EPUB3GetTestPathForFileNamed(char *dest, const char *name)
{
  (void)strcpy(dest, TEST_DATA_PATH);
  (void)strncat(dest, name, strlen(name));
}

#endif
