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
} EPUB3Error;

typedef struct EPUB3 * EPUB3Ref;
typedef struct EPUB3Metadata * EPUB3MetadataRef;
typedef enum {
  kEPUB3Version_201 = 201,
  kEPUB3Version_3   = 300,
} EPUB3Version;

void EPUB3Retain(EPUB3Ref epub);
void EPUB3Release(EPUB3Ref epub);
void EPUB3MetadataRetain(EPUB3MetadataRef metadata);
void EPUB3MetadataRelease(EPUB3MetadataRef metadata);

EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub);
void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title);
char * EPUB3CopyMetadataTitle(EPUB3MetadataRef metadata);
void EPUB3MetadataSetIdentifier(EPUB3MetadataRef metadata, const char * identifier);
char * EPUB3CopyMetadataIdentifier(EPUB3MetadataRef metadata);
void EPUB3MetadataSetLanguage(EPUB3MetadataRef metadata, const char * language);
char * EPUB3CopyMetadataLanguage(EPUB3MetadataRef metadata);

EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path);


#if defined(__cplusplus)
} //EXTERN "C"
#endif

#endif
