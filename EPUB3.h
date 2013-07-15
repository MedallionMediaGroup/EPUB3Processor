#ifndef EPUB3_h
#define EPUB3_h

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>

typedef enum _EPUB3Error {
  kEPUB3Success = 0,
  kEPUB3UnknownError = 1001,
  kEPUB3InvalidArgumentError = 1002,
  kEPUB3InvalidMimetypeError = 1003,
  kEPUB3FileNotFoundInArchiveError = 1004,
  kEPUB3FileReadFromArchiveError = 1005,
  kEPUB3ArchiveUnavailableError = 1006,
  kEPUB3XMLReadFromBufferError = 1007,
  kEPUB3XMLParseError = 1008,
  kEPUB3XMLXElementNotFoundError = 1009,
  kEPUB3XMLXDocumentInvalidError = 1010,
  kEPUB3NCXNavMapEnd = 1011,
} EPUB3Error;

typedef enum { kEPUB3_NO = 0 , kEPUB3_YES = 1 } EPUB3Bool;

typedef struct EPUB3 * EPUB3Ref;
typedef struct EPUB3TocItem * EPUB3TocItemRef;

EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path, EPUB3Error *error);
void EPUB3Retain(EPUB3Ref epub);
void EPUB3Release(EPUB3Ref epub);
char * EPUB3CopyTitle(EPUB3Ref epub);
char * EPUB3CopyIdentifier(EPUB3Ref epub);
char * EPUB3CopyLanguage(EPUB3Ref epub);
char * EPUB3CopyCoverImagePath(EPUB3Ref epub);
EPUB3Error EPUB3CopyCoverImage(EPUB3Ref epub, void ** bytes, uint32_t * byteCount);
int32_t EPUB3CountOfSequentialResources(EPUB3Ref epub);
EPUB3Error EPUB3GetPathsOfSequentialResources(EPUB3Ref epub, const char ** resources);
EPUB3Error EPUB3ExtractArchiveToPath(EPUB3Ref epub, const char * path);
EPUB3Error EPUB3CopyRootFilePathFromContainer(EPUB3Ref epub, char ** rootPath);

int32_t EPUB3CountOfTocRootItems(EPUB3Ref epub);
EPUB3Error EPUB3GetTocRootItems(EPUB3Ref epub, EPUB3TocItemRef *tocItems);
EPUB3Bool EPUB3TocItemHasParent(EPUB3TocItemRef tocItem);
EPUB3TocItemRef EPUB3TocItemGetParent(EPUB3TocItemRef tocItem);
int32_t EPUB3TocItemCountOfChildren(EPUB3TocItemRef tocItem);
EPUB3Error EPUB3TocItemGetChildren(EPUB3TocItemRef parent, EPUB3TocItemRef *children);
char * EPUB3TocItemCopyTitle(EPUB3TocItemRef tocItem);
char * EPUB3TocItemCopyPath(EPUB3TocItemRef tocItem);


#if defined(__cplusplus)
} //EXTERN "C"
#endif

#endif
