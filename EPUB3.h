#ifndef EPUB3_h
#define EPUB3_h

#if defined(__cplusplus)
extern "C" {
#endif
  
#include <stdint.h>

typedef uint32_t EPUB3Error;

enum _EPUB3Error {
  kEPUB3Success = 0,
  kEPUB3InvalidMimetypeError = 1001
};
  
#define E3_INVALID_MIMETYPE UINT32_C(1)
  
typedef struct EPUB3 * EPUB3Ref;
typedef struct EPUB3Metadata * EPUB3MetadataRef;

void EPUB3Retain(EPUB3Ref epub);
void EPUB3Release(EPUB3Ref epub);
void EPUB3MetadataRetain(EPUB3MetadataRef metadata);
void EPUB3MetadataRelease(EPUB3MetadataRef metadata);

EPUB3MetadataRef EPUB3CopyMetadata(EPUB3Ref epub);
void EPUB3MetadataSetTitle(EPUB3MetadataRef metadata, const char * title);
char * EPUB3CopyMetadataTitle(EPUB3MetadataRef metadata);

EPUB3Ref EPUB3CreateWithArchiveAtPath(const char * path);


#if defined(__cplusplus)
} //EXTERN "C"
#endif

#endif
