#ifndef EPUB3Processor_test_common_h
#define EPUB3Processor_test_common_h

#include <inttypes.h>
#include <stdlib.h>

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define TEST_EXPORT __attribute__((visibility("default")))

#ifndef TEST_DATA_PATH
#define TEST_DATA_PATH "/tmp/"
#endif

#define TEST_PATH_VAR_FOR_FILENAME(__varname, __filename) \
  uint64_t __length = EPUB3TestPathLengthForFileNamed(__filename); \
  char __varname[__length]; \
  (void)EPUB3GetTestPathForFileNamed(path, __filename);


static uint64_t EPUB3TestPathLengthForFileNamed(const char * name) {
  return strlen(TEST_DATA_PATH) + strlen(name) + 1;
}

static void EPUB3GetTestPathForFileNamed(char *dest, const char *name)
{
  (void)strcpy(dest, TEST_DATA_PATH);
  (void)strncat(dest, name, strlen(name));
}

#endif
